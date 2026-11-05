#pragma once
#include <brpc/channel.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include "logger.hpp"

namespace bite_im {

/* ============================================================
 * 1. 封装单个服务的信道管理类 (ServiceChannel)
 * 职责：维护某一种服务（如 user_service）的多个可用节点（host），
 *      并在需要发起 RPC 时，按轮询（Round-Robin）策略返回一个连接。
 * ============================================================ */
class ServiceChannel {
public:
    using ptr = std::shared_ptr<ServiceChannel>;
    using ChannelPtr = std::shared_ptr<brpc::Channel>;

    ServiceChannel(const std::string &name) : _service_name(name), _index(0) {}

    // 服务上线了一个节点，则调用 append 新增信道
    void append(const std::string &host) {
        // 1. 创建 brpc 信道并配置参数
        auto channel = std::make_shared<brpc::Channel>();
        brpc::ChannelOptions options;
        options.connect_timeout_ms = -1; // 连接超时无限等待
        options.timeout_ms = -1;         // RPC调用超时无限等待
        options.max_retry = 3;           // 失败重试 3 次
        options.protocol = "baidu_std";  // brpc 默认协议
        
        // 2. 用 host 初始化信道
        int ret = channel->Init(host.c_str(), &options);
        if (ret == -1) {
            LOG_ERROR("初始化{}-{}信道失败!", _service_name, host);
            return;
        }

        // 3. 加锁加入管理容器
        std::unique_lock<std::mutex> lock(_mutex);
        _hosts.insert(std::make_pair(host, channel)); // 映射表：host -> channel
        _channels.push_back(channel);                 // 数组：用于轮询
        LOG_DEBUG("{}-{} 服务上线新节点，添加信道成功！", _service_name, host);
    }

    // 服务下线了一个节点，则调用 remove 释放信道
    void remove(const std::string &host) {
        std::unique_lock<std::mutex> lock(_mutex);
        auto it = _hosts.find(host);
        if (it == _hosts.end()) {
            LOG_WARN("{}-{}节点删除信道时，没有找到信道信息！", _service_name, host);
            return;
        }

        // 1. 从轮询数组中删除（时间复杂度 O(N)）
        for (auto vit = _channels.begin(); vit != _channels.end(); ++vit) {
            if (*vit == it->second) { // 匹配指针
                _channels.erase(vit);
                break;
            }
        }
        // 2. 从映射表中删除
        _hosts.erase(it);
        LOG_DEBUG("{}-{} 服务下线，移除信道成功！", _service_name, host);
    }

    // 通过 RR 轮转策略，获取一个 Channel 用于发起 RPC 调用
    ChannelPtr choose() {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_channels.empty()) {
            LOG_ERROR("当前没有能够提供 {} 服务的节点！", _service_name);
            return ChannelPtr(); // 返回空指针
        }
        // 轮询核心：每次 index 加 1，取模保证不越界
        int32_t idx = _index++ % _channels.size();
        return _channels[idx];
    }

private:
    std::mutex _mutex;
    int32_t _index; // 当前轮转下标计数器
    std::string _service_name; // 服务名称
    std::vector<ChannelPtr> _channels; // 用来做负载均衡的集合
    std::unordered_map<std::string, ChannelPtr> _hosts; // 主机地址 -> 信道，用于快速删除
};

/* ============================================================
 * 2. 总体的服务信道管理类 (ServiceManager)
 * 职责：管理所有关注的服务（如 user, friend, file），
 *      接收服务发现模块（etcd）传来的上下线事件，并分发给对应的 ServiceChannel。
 * ============================================================ */
class ServiceManager {
public:
    using ptr = std::shared_ptr<ServiceManager>;
    ServiceManager() {}

    // 获取指定服务的节点信道（供业务层发起 RPC 调用）
    ServiceChannel::ChannelPtr choose(const std::string &service_name) {
        std::unique_lock<std::mutex> lock(_mutex);
        auto sit = _services.find(service_name);
        if (sit == _services.end()) {
            LOG_ERROR("当前没有能够提供 {} 服务的节点！", service_name);
            return ServiceChannel::ChannelPtr();
        }
        return sit->second->choose();
    }

    // 声明关注哪些服务，不关心的服务就算上线了也不管
    void declared(const std::string &service_name) {
        std::unique_lock<std::mutex> lock(_mutex);
        _follow_services.insert(service_name);
    }

    // 【回调】服务上线事件：由 Discovery 模块调用
    void onServiceOnline(const std::string &service_instance, const std::string &host) {
        // 从完整的实例路径（如 /service/user/instance1）解析出服务名（/service/user）
        std::string service_name = getServiceName(service_instance);
        ServiceChannel::ptr service;

        {
            std::unique_lock<std::mutex> lock(_mutex);
            // 1. 如果是不关心的服务，直接返回
            auto fit = _follow_services.find(service_name);
            if (fit == _follow_services.end()) {
                return;
            }
            // 2. 获取该服务的管理器，没有就新建
            auto sit = _services.find(service_name);
            if (sit == _services.end()) {
                service = std::make_shared<ServiceChannel>(service_name);
                _services.insert(std::make_pair(service_name, service));
            } else {
                service = sit->second;
            }
        } // 离开作用域解锁，防止后续操作阻塞其他服务查询

        // 3. 将节点添加到对应的 ServiceChannel 中
        if (!service) {
            LOG_ERROR("新增 {} 服务管理节点失败！", service_name);
            return;
        }
        service->append(host);
    }

    // 【回调】服务下线事件：由 Discovery 模块调用
    void onServiceOffline(const std::string &service_instance, const std::string &host) {
        std::string service_name = getServiceName(service_instance);
        ServiceChannel::ptr service;

        {
            std::unique_lock<std::mutex> lock(_mutex);
            // 1. 不关心的服务忽略
            auto fit = _follow_services.find(service_name);
            if (fit == _follow_services.end()) {
                return;
            }
            // 2. 获取该服务的管理器
            auto sit = _services.find(service_name);
            if (sit == _services.end()) {
                LOG_WARN("删除{}服务节点时，没有找到管理对象", service_name);
                return;
            }
            service = sit->second;
        } // 同样缩小锁的粒度

        // 3. 从 ServiceChannel 中移除节点
        service->remove(host);
    }

private:
    // 解析 etcd 实例路径（如 "/service/user/instance1" -> 截取出 "/service/user"）
    std::string getServiceName(const std::string &service_instance) {
        auto pos = service_instance.find_last_of('/');
        if (pos == std::string::npos) return service_instance;
        return service_instance.substr(0, pos);
    }

private:
    std::mutex _mutex;
    std::unordered_set<std::string> _follow_services; // 我关心的服务名列表
    std::unordered_map<std::string, ServiceChannel::ptr> _services; // 服务名 -> 信道管理器
};

} // namespace bite_im