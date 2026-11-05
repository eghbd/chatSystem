// 实现文件存储子服务
// 1. 实现文件 rpc 服务类 —— 实现 rpc 调用的业务处理接口
// 2. 实现文件存储子服务的服务器类
// 3. 实现文件存储子服务类的构造者
#include <brpc/server.h>
#include <butil/logging.h>

#include "etcd.hpp"     // 服务注册模块封装
#include "logger.hpp"   // 日志模块封装
#include "utils.hpp"
#include "base.pb.h"
#include "file.pb.h"

namespace bite_im {

/* ============================================================
 * 一、业务处理层：FileServiceImpl
 *   继承 protobuf 生成的服务基类 FileService，
 *   实现 4 个 RPC 方法：单文件上传/下载、多文件上传/下载
 * ============================================================ */
class FileServiceImpl : public bite_im::FileService {
public:
    // 构造：初始化存储目录
    FileServiceImpl(const std::string &storage_path)
        : _storage_path(storage_path) {
        umask(0);                                    // 清除 umask 掩码，保证 mkdir 权限不被削减
        mkdir(storage_path.c_str(), 0775);           // 创建存储目录（已存在则失败，忽略）
        if (_storage_path.back() != '/')             // 补齐尾部 '/'，保证后续路径拼接正确
            _storage_path.push_back('/');
    }

    ~FileServiceImpl() {}

    /* ---------- 1. 单文件下载 ---------- */
    void GetSingleFile(google::protobuf::RpcController* controller,
                       const ::bite_im::GetSingleFileReq* request,
                       ::bite_im::GetSingleFileRsp* response,
                       ::google::protobuf::Closure* done) {
        // RAII 守卫：函数返回时自动 done->Run()，任何分支都不会漏响应
        brpc::ClosureGuard rpc_guard(done);
        // 透传请求 ID，便于全链路日志追踪
        response->set_request_id(request->request_id());

        // 1. 取出请求中的文件 ID（本质就是磁盘文件名）
        std::string fid = request->file_id();
        std::string filename = _storage_path + fid;

        // 2. 以文件 ID 为文件名，读取文件数据
        std::string body;
        bool ret = readFile(filename, body);
        if (ret == false) {
            response->set_success(false);
            response->set_errmsg("读取文件数据失败！");
            LOG_ERROR("{} 读取文件数据失败！", request->request_id());
            return;
        }

        // 3. 组织响应：回填 file_id 与 file_content
        response->set_success(true);
        response->mutable_file_data()->set_file_id(fid);
        response->mutable_file_data()->set_file_content(body);
    }

    /* ---------- 2. 多文件下载 ---------- */
    void GetMultiFile(google::protobuf::RpcController* controller,
                      const ::bite_im::GetMultiFileReq* request,
                      ::bite_im::GetMultiFileRsp* response,
                      ::google::protobuf::Closure* done) {
        brpc::ClosureGuard rpc_guard(done);
        response->set_request_id(request->request_id());

        // 循环取出请求中的每个文件 ID，读取后填充到 map
        for (int i = 0; i < request->file_id_list_size(); i++) {
            std::string fid = request->file_id_list(i);
            std::string filename = _storage_path + fid;

            std::string body;
            bool ret = readFile(filename, body);
            if (ret == false) {
                // 任一文件读取失败 → 整体失败（不返回部分结果）
                response->set_success(false);
                response->set_errmsg("读取文件数据失败！");
                LOG_ERROR("{} 读取文件数据失败！", request->request_id());
                return;
            }

            FileDownloadData data;
            data.set_file_id(fid);
            data.set_file_content(body);
            response->mutable_file_data()->insert({fid, data});   // map<fid, data>
        }
        response->set_success(true);
    }

    /* ---------- 3. 单文件上传 ---------- */
    void PutSingleFile(google::protobuf::RpcController* controller,
                       const ::bite_im::PutSingleFileReq* request,
                       ::bite_im::PutSingleFileRsp* response,
                       ::google::protobuf::Closure* done) {
        brpc::ClosureGuard rpc_guard(done);
        response->set_request_id(request->request_id());

        // 1. 生成唯一 uuid 作为文件名（即文件 ID）
        std::string fid = uuid();
        std::string filename = _storage_path + fid;

        // 2. 取出请求中的文件数据，写入磁盘
        bool ret = writeFile(filename, request->file_data().file_content());
        if (ret == false) {
            unlink(filename.c_str());   // 删除可能残留的半截文件
            response->set_success(false);
            response->set_errmsg("写入文件数据失败！");   // 修正原文案（原为“读取”）
            LOG_ERROR("{} 写入文件数据失败！", request->request_id());
            return;
        }

        // 3. 组织响应：返回文件元信息
        response->set_success(true);
        response->mutable_file_info()->set_file_id(fid);
        response->mutable_file_info()->set_file_size(request->file_data().file_size());
        response->mutable_file_info()->set_file_name(request->file_data().file_name());
    }

    /* ---------- 4. 多文件上传 ---------- */
    void PutMultiFile(google::protobuf::RpcController* controller,
                      const ::bite_im::PutMultiFileReq* request,
                      ::bite_im::PutMultiFileRsp* response,
                      ::google::protobuf::Closure* done) {
        brpc::ClosureGuard rpc_guard(done);
        response->set_request_id(request->request_id());

        for (int i = 0; i < request->file_data_size(); i++) {
            // 每个文件都生成独立 uuid 并落盘
            std::string fid = uuid();
            std::string filename = _storage_path + fid;

            bool ret = writeFile(filename, request->file_data(i).file_content());
            if (ret == false) {
                response->set_success(false);
                response->set_errmsg("写入文件数据失败！");   // 修正原文案
                LOG_ERROR("{} 写入文件数据失败！", request->request_id());
                return;
            }

            // 追加一条文件元信息到响应
            bite_im::FileMessageInfo *info = response->add_file_info();
            info->set_file_id(fid);
            info->set_file_size(request->file_data(i).file_size());
            info->set_file_name(request->file_data(i).file_name());
        }
        response->set_success(true);
    }

private:
    std::string _storage_path;   // 文件存储根目录（尾部带 '/'）
};


/* ============================================================
 * 二、服务器封装层：FileServer
 *   聚合 Etcd 注册客户端 + 已构建好的 brpc::Server
 * ============================================================ */
class FileServer {
public:
    using ptr = std::shared_ptr<FileServer>;

    FileServer(const Registry::ptr &reg_client,
               const std::shared_ptr<brpc::Server> &server)
        : _reg_client(reg_client), _rpc_server(server) {}

    ~FileServer() {}

    // 启动 RPC 服务器，阻塞进入事件循环
    void start() {
        _rpc_server->RunUntilAskedToQuit();
    }

private:
    Registry::ptr _reg_client;                 // Etcd 服务注册客户端
    std::shared_ptr<brpc::Server> _rpc_server; // bRPC 服务器
};


/* ============================================================
 * 三、建造者层：FileServerBuilder
 *   分步组装：注册客户端 + RPC 服务器 → build 出 FileServer
 * ============================================================ */
class FileServerBuilder {
public:
    // 构造服务注册客户端，并以 service_name 注册 access_host 到 Etcd
    void make_reg_object(const std::string &reg_host,
                         const std::string &service_name,
                         const std::string &access_host) {
        _reg_client = std::make_shared<Registry>(reg_host);
        _reg_client->registry(service_name, access_host);
    }

    // 构造 RPC 服务器：创建 Service 实现、添加服务、启动监听
    void make_rpc_server(uint16_t port, int32_t timeout,
                         uint8_t num_threads,
                         const std::string &path = "./data/") {
        _rpc_server = std::make_shared<brpc::Server>();

        // 服务对象所有权交给 brpc，由其负责 delete
        FileServiceImpl *file_service = new FileServiceImpl(path);
        int ret = _rpc_server->AddService(
            file_service, brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
        if (ret == -1) {
            LOG_ERROR("添加 Rpc 服务失败！");
            abort();
        }

        brpc::ServerOptions options;
        options.idle_timeout_sec = timeout;   // 空闲连接超时
        options.num_threads      = num_threads; // 工作线程数

        ret = _rpc_server->Start(port, &options);
        if (ret == -1) {
            LOG_ERROR("服务启动失败！");
            abort();
        }
    }

    // 组装并返回最终服务器对象
    FileServer::ptr build() {
        if (!_reg_client) {
            LOG_ERROR("还未初始化服务注册模块！");
            abort();
        }
        if (!_rpc_server) {
            LOG_ERROR("还未初始化 RPC 服务器模块！");
            abort();
        }
        FileServer::ptr server =
            std::make_shared<FileServer>(_reg_client, _rpc_server);
        return server;
    }

private:
    Registry::ptr _reg_client; // Etcd 服务注册客户端
    std::shared_ptr<brpc::Server> _rpc_server; // bRPC 服务器
};

} // namespace bite_im