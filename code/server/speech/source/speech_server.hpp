//实现语音识别子服务
#include <brpc/server.h>
#include <butil/logging.h>

#include "asr.hpp"      // 语音识别模块封装
#include "etcd.hpp"     // 服务注册模块封装
#include "logger.hpp"   // 日志模块封装
#include "speech.pb.h"  // protobuf框架代码
namespace bite_im{   // 命名空间：聊天 IM 业务模块

//////////////////////////////////////////////////////////////////
// SpeechServiceImpl：语音识别 RPC 服务实现类
// 继承自 protobuf 生成的 bite_im::SpeechService 基类，
// 具体实现 SpeechRecognition 这个 RPC 方法。
/////////////////////////////////////////////////////////////////
class SpeechServiceImpl : public bite_im::SpeechService {
    public:
        // 构造函数：注入语音识别客户端（依赖注入）
        SpeechServiceImpl(const ASRClient::ptr &asr_client):
            _asr_client(asr_client){}
        ~SpeechServiceImpl(){}

        // RPC 方法实现：语音转文字
        // 参数（brpc/protobuf 约定）：
        //   controller - RPC 控制器，用于设置/获取 RPC 层状态
        //   request    - 客户端请求（含 request_id、speech_content 音频数据）
        //   response   - 服务端响应（含 success、errmsg、recognition_result）
        //   done       - 响应回调闭包，调用 done->Run() 才会把响应发回客户端
        void SpeechRecognition(google::protobuf::RpcController* controller,
                       const ::bite_im::SpeechRecognitionReq* request,
                       ::bite_im::SpeechRecognitionRsp* response,
                       ::google::protobuf::Closure* done) {
            LOG_DEBUG("收到语音转文字请求！");

            // RAII 守卫：无论函数从哪个路径返回，析构时都会自动调用 done->Run()，
            // 防止忘记回调导致客户端请求悬挂。本方法是同步逻辑，无需 release。
            brpc::ClosureGuard rpc_guard(done);

            // 1. 获取语音数据（从请求中取音频内容）
            // 2. 调用百度语音识别接口进行识别
            std::string err;   // 输出参数：识别失败时的错误信息
            std::string res = _asr_client->recognize(request->speech_content(), err);

            // 识别失败（返回空字符串）时的处理
            if (res.empty()) {
                LOG_ERROR("{} 语音识别失败！", request->request_id());
                response->set_request_id(request->request_id());   // 回填请求 ID，便于链路追踪
                response->set_success(false);                      // 标记失败
                response->set_errmsg("语音识别失败:" + err);        // 回填错误信息
                return;   // 直接返回，rpc_guard 析构时自动 done->Run() 发送响应
            }

            // 3. 组织成功响应
            response->set_request_id(request->request_id());
            response->set_success(true);
            response->set_recognition_result(res);   // 填入识别出的文本
            // 函数结束，rpc_guard 析构 → 自动 done->Run() → 响应发给客户端
        }
    private:
        ASRClient::ptr _asr_client;   // 语音识别客户端（持有依赖）
};


/////////////////////////////////////////////////////////////////////
// SpeechServer：RPC 服务器运行封装类
// 持有 ASR 客户端、服务注册客户端、brpc::Server 三个组件，
// 对外只暴露 start() 用于启动并阻塞运行服务。
//////////////////////////////////////////////////////////////////////
class SpeechServer {
    public:
        using ptr = std::shared_ptr<SpeechServer>;

        // 构造函数：注入三个依赖组件
        SpeechServer(const ASRClient::ptr asr_client,
            const Registry::ptr &reg_client,
            const std::shared_ptr<brpc::Server> &server):
            _asr_client(asr_client),
            _reg_client(reg_client),
            _rpc_server(server){}
        ~SpeechServer(){}

        // 搭建 RPC 服务器并启动：阻塞运行，直到被要求退出
        void start() {
            _rpc_server->RunUntilAskedToQuit();
        }
    private:
        ASRClient::ptr _asr_client;                 // 语音识别客户端
        Registry::ptr _reg_client;                  // 服务注册中心客户端
        std::shared_ptr<brpc::Server> _rpc_server;  // brpc 服务器对象
};


//////////////////////////////////////////////////////////////////////
// SpeechServerBuilder：语音服务建造者（Builder 模式）
// 分步构造各个子组件，最终通过 build() 产出 SpeechServer 对象。
////////////////////////////////////////////////////////////////////
class SpeechServerBuilder {
    public:
        // 构造语音识别客户端对象（百度 AI 三元组凭据）
        void make_asr_object(const std::string &app_id,
            const std::string &api_key,
            const std::string &secret_key) {
            _asr_client = std::make_shared<ASRClient>(app_id, api_key, secret_key);
        }

        // 构造服务注册客户端对象，并立即向注册中心注册本服务
        void make_reg_object(const std::string &reg_host,       // 注册中心地址
            const std::string &service_name,                    // 服务名
            const std::string &access_host) {                   // 本机可访问地址
            _reg_client = std::make_shared<Registry>(reg_host);
            _reg_client->registry(service_name, access_host);
        }

        // 构造 RPC 服务器对象：添加服务、配置参数、启动监听
        void make_rpc_server(uint16_t port, int32_t timeout, uint8_t num_threads) {
            // 前置校验：必须先初始化 ASR 客户端
            if (!_asr_client) {
                LOG_ERROR("还未初始化语音识别模块！");
                abort();
            }
            _rpc_server = std::make_shared<brpc::Server>();

            // 创建服务实现对象并注册到 brpc::Server
            SpeechServiceImpl *speech_service = new SpeechServiceImpl(_asr_client);
            // SERVER_OWNS_SERVICE：由 brpc::Server 负责该 service 的生命周期（delete）
            int ret = _rpc_server->AddService(speech_service,
                brpc::ServiceOwnership::SERVER_OWNS_SERVICE);
            if (ret == -1) {
                LOG_ERROR("添加Rpc服务失败！");
                abort();
            }

            // 配置服务器选项
            brpc::ServerOptions options;
            options.idle_timeout_sec = timeout;   // 空闲连接超时时间（秒）
            options.num_threads = num_threads;    // 工作线程数

            // 在指定端口启动监听
            ret = _rpc_server->Start(port, &options);
            if (ret == -1) {
                LOG_ERROR("服务启动失败！");
                abort();
            }
        }

        // 构建最终的 SpeechServer 对象
        // 校验三大组件是否都已初始化，否则 abort
        SpeechServer::ptr build() {
            if (!_asr_client) {
                LOG_ERROR("还未初始化语音识别模块！");
                abort();
            }
            if (!_reg_client) {
                LOG_ERROR("还未初始化服务注册模块！");
                abort();
            }
            if (!_rpc_server) {
                LOG_ERROR("还未初始化RPC服务器模块！");
                abort();
            }
            SpeechServer::ptr server = std::make_shared<SpeechServer>(
                _asr_client, _reg_client, _rpc_server);
            return server;
        }
    private:
        ASRClient::ptr _asr_client;                 // 语音识别客户端
        Registry::ptr _reg_client;                  // 服务注册客户端
        std::shared_ptr<brpc::Server> _rpc_server;  // RPC 服务器
};
}