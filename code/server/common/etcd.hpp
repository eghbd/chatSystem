#pragma once
#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Response.hpp>
#include <etcd/Watcher.hpp>
#include <etcd/Value.hpp>
#include <functional>
#include "logger.hpp"

namespace bite_im{
//服务注册客户端类
class Registry {
    public:
        using ptr = std::shared_ptr<Registry>;
        Registry(const std::string &host):
            _client(std::make_shared<etcd::Client>(host)) ,
            _keep_alive(_client->leasekeepalive(3).get()),
            _lease_id(_keep_alive->Lease()){}
        ~Registry() { _keep_alive->Cancel(); }
        bool registry(const std::string &key, const std::string &val) {
            auto resp = _client->put(key, val, _lease_id).get();
            if (resp.is_ok() == false) {
                LOG_ERROR("注册数据失败：{}", resp.error_message());
                return false;
            }
            return true;
        }
    private:
        std::shared_ptr<etcd::Client> _client;
        std::shared_ptr<etcd::KeepAlive> _keep_alive;
        uint64_t _lease_id;
};

//服务发现客户端类
class Discovery {
public:
    using ptr = std::shared_ptr<Discovery>;

    // 回调类型：参数为 (key, value)
    // 例：("/service/user/instance1", "127.0.0.1:9001")
    using NotifyCallback = std::function<void(std::string, std::string)>;

    Discovery(const std::string &host,          // etcd 服务器地址
              const std::string &basedir,       // 监听的目录前缀，如 "/service/user"
              const NotifyCallback &put_cb,     // 服务上线/变更回调
              const NotifyCallback &del_cb)     // 服务下线回调
        : _client(std::make_shared<etcd::Client>(host)),
          _put_cb(put_cb), _del_cb(del_cb) {

        /* ---------- 阶段 1：服务发现初始同步 ---------- */
        // 先拉取 basedir 下当前已有的所有 key（即已注册的实例）
        auto resp = _client->ls(basedir).get();   // .get() 同步等待结果
        if (resp.is_ok() == false) {
            LOG_ERROR("获取服务信息数据失败：{}", resp.error_message());
        }
        int sz = resp.keys().size();
        for (int i = 0; i < sz; ++i) {
            // 把已有实例逐个通过 put_cb 交给上层缓存
            // key = "/service/user/instance1"，value = "127.0.0.1:9001"
            if (_put_cb) _put_cb(resp.key(i), resp.value(i).as_string());
        }

        /* ---------- 阶段 2：事件监听 ---------- */
        // 创建 Watcher，持续监听 basedir 的 PUT / DELETE 事件
        // 第 4 个参数 true：从当前版本开始监听（不重放历史事件）
        _watcher = std::make_shared<etcd::Watcher>(
            *_client.get(), basedir,
            std::bind(&Discovery::callback, this, std::placeholders::_1),
            true);
    }

    ~Discovery() {
        _watcher->Cancel();   // 析构时取消监听，避免回调访问已销毁对象
    }

private:
    // Watcher 收到事件后统一走这里
    void callback(const etcd::Response &resp) {
        if (resp.is_ok() == false) {
            LOG_ERROR("收到一个错误的事件通知: {}", resp.error_message());
            return;
        }
        // 一个响应可能包含多个事件，逐个处理
        for (auto const &ev : resp.events()) {
            if (ev.event_type() == etcd::Event::EventType::PUT) {
                // 服务上线或地址变更
                if (_put_cb) _put_cb(ev.kv().key(), ev.kv().as_string());
                LOG_DEBUG("新增服务：{}-{}", ev.kv().key(), ev.kv().as_string());
            } else if (ev.event_type() == etcd::Event::EventType::DELETE_) {
                // 服务下线：注意用 prev_kv() 取被删前的值，kv() 此时已为空
                if (_del_cb) _del_cb(ev.prev_kv().key(), ev.prev_kv().as_string());
                LOG_DEBUG("下线服务：{}-{}", ev.prev_kv().key(), ev.prev_kv().as_string());
            }
        }
    }

private:
    NotifyCallback _put_cb;                      // 上线回调
    NotifyCallback _del_cb;                      // 下线回调
    std::shared_ptr<etcd::Client>  _client;      // etcd 客户端
    std::shared_ptr<etcd::Watcher> _watcher;     // 事件监听器
};
}