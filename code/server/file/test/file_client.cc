//编写一个file客户端程序，对文件存储子服务进行单元测试
// 1. 封装四个接口进行rpc调用，实现对于四个业务接口的测试
#include <gflags/gflags.h>       // Google 命令行参数解析库
#include <gtest/gtest.h>         // Google C++ 单元测试框架
#include <thread>                // 多线程库（用于等待服务上线时的休眠）
#include "etcd.hpp"              // etcd 服务发现模块封装
#include "channel.hpp"           // brpc 信道管理封装
#include "logger.hpp"            // 日志模块封装
#include "file.pb.h"             // 文件服务 protobuf 生成的协议代码
#include "base.pb.h"             // 基础协议（如用户信息等）
#include "utils.hpp"             // 工具类封装（如读取/写入文件等）

// ==================== 定义命令行参数 ====================
DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

// 微服务基础设施配置
DEFINE_string(etcd_host, "http://127.0.0.1:2379", "服务注册中心地址"); // etcd 的访问地址
DEFINE_string(base_service, "/service", "服务监控根目录");              // etcd 中存放服务的根路径
DEFINE_string(file_service, "/service/file_service", "文件服务路径");   // 要找的目标服务在 etcd 中的具体路径


// ==================== 全局变量 ====================
// 信道管理指针（全局）。用于后续所有的 RPC 调用，保证整个测试程序共用同一个连接池
bite_im::ServiceChannel::ChannelPtr channel; 
// 全局变量：保存单文件测试中上传文件后返回的 file_id，供下载测试用例使用
std::string single_file_id;


// ==================== 测试用例 1：单文件上传测试 ====================
TEST(put_test, single_file) {
    // 1. 读取当前目录下的指定文件数据（这里以 Makefile 为例）
    std::string body;
    // 使用自己封装的 utils 读取文件内容到 body 字符串中，ASSERT_TRUE 如果失败直接终止当前测试用例
    ASSERT_TRUE(bite_im::readFile("./Makefile", body)); 

    // 2. 实例化rpc调用客户端对象（Stub 桩），发起rpc调用
    // 注意：这里依赖全局变量 channel 已经成功连接到服务端
    bite_im::FileService_Stub stub(channel.get());

    // 构造 Protobuf 请求对象
    bite_im::PutSingleFileReq req;
    req.set_request_id("1111"); // 设置请求 ID，用于链路追踪
    
    // 设置文件元数据和文件内容
    req.mutable_file_data()->set_file_name("Makefile");
    req.mutable_file_data()->set_file_size(body.size());
    req.mutable_file_data()->set_file_content(body);

    // brpc 的 Controller 和 Response 对象需要 new 出来（这里示例代码存在内存泄漏风险，测试用例中勉强可以接受）
    auto cntl = std::make_unique<brpc::Controller>();
    auto rsp = std::make_unique<bite_im::PutSingleFileRsp>();
    
    // 发起同步 RPC 调用，最后一个参数为 nullptr 表示同步调用
    stub.PutSingleFile(cntl.get(), &req, rsp.get(), nullptr);
    
    // 检查网络层/框架层是否调用失败（如网络超时、连接断开）
    ASSERT_FALSE(cntl->Failed());
    
    // 3. 检测业务返回值中上传是否成功
    ASSERT_TRUE(rsp->success()); // 业务逻辑是否成功
    ASSERT_EQ(rsp->file_info().file_size(), body.size()); // 校验服务端记录的文件大小和本地一致
    ASSERT_EQ(rsp->file_info().file_name(), "Makefile"); // 校验文件名一致
    
    // 将服务端生成的唯一文件 ID 保存到全局变量，给下一个下载测试用例用
    single_file_id = rsp->file_info().file_id(); 
    LOG_DEBUG("文件ID：{}", rsp->file_info().file_id()); // 打印日志方便调试
}


// ==================== 测试用例 2：单文件下载测试 ====================
TEST(get_test, single_file) {
    // 先发起Rpc调用，进行文件下载
    bite_im::FileService_Stub stub(channel.get());
    
    // 构造请求
    bite_im::GetSingleFileReq req;

    req.set_request_id("2222"); // 请求 ID
    req.set_file_id(single_file_id); // 使用上一个测试用例上传成功后保存的文件 ID

    // 实例化框架对象，准备调用
    auto cntl = std::make_unique<brpc::Controller>();
    auto rsp = std::make_unique<bite_im::GetSingleFileRsp>();
    stub.GetSingleFile(cntl.get(), &req, rsp.get(), nullptr);
    
    // 校验 RPC 框架和业务是否成功
    ASSERT_FALSE(cntl->Failed());
    ASSERT_TRUE(rsp->success());
    
    // 将文件数据存储到本地文件中，验证文件 ID 是否一致
    ASSERT_EQ(single_file_id, rsp->file_data().file_id());
    
    // 调用自己封装的工具函数，将服务端返回的 file_content 写入本地文件 "make_file_download"
    bite_im::writeFile("make_file_download", rsp->file_data().file_content());
}


// 全局变量：保存多文件测试中上传文件后返回的 file_id 列表
std::vector<std::string> multi_file_id;

// ==================== 测试用例 3：多文件上传测试 ====================
TEST(put_test, multi_file) {
    // 1. 读取当前目录下的两个指定文件数据
    std::string body1;
    ASSERT_TRUE(bite_im::readFile("./base.pb.h", body1));
    std::string body2;
    ASSERT_TRUE(bite_im::readFile("./file.pb.h", body2));
    
    // 2. 实例化rpc调用客户端对象，发起rpc调用
    bite_im::FileService_Stub stub(channel.get());

    // 构造多文件上传请求
    bite_im::PutMultiFileReq req;
    req.set_request_id("3333");

    // 添加第一个文件的数据（通过 add_file_data() 返回指针进行操作）
    auto file_data = req.add_file_data();
    file_data->set_file_name("base.pb.h");
    file_data->set_file_size(body1.size());
    file_data->set_file_content(body1);

    // 添加第二个文件的数据
    file_data = req.add_file_data();
    file_data->set_file_name("file.pb.h");
    file_data->set_file_size(body2.size());
    file_data->set_file_content(body2);

    // 发起 RPC 调用
    auto cntl = std::make_unique<brpc::Controller>();
    auto rsp = std::make_unique<bite_im::PutMultiFileRsp>();
    stub.PutMultiFile(cntl.get(), &req, rsp.get(), nullptr);
    
    ASSERT_FALSE(cntl->Failed());
    
    // 3. 检测返回值中上传是否成功
    ASSERT_TRUE(rsp->success());
    
    // 遍历返回的文件信息列表，把生成的 file_id 存入全局变量供下载测试使用
    for (int i = 0; i < rsp->file_info_size(); i++){
        multi_file_id.push_back(rsp->file_info(i).file_id());
        LOG_DEBUG("文件ID：{}", multi_file_id[i]);
    }
}


// ==================== 测试用例 4：多文件下载测试 ====================
TEST(get_test, multi_file) {
    // 先发起Rpc调用，进行文件下载
    bite_im::FileService_Stub stub(channel.get());
    
    // 构造多文件下载请求
    bite_im::GetMultiFileReq req;
    req.set_request_id("4444");
    
    // 把之前保存的多个文件 ID 塞入请求列表
    req.add_file_id_list(multi_file_id[0]);
    req.add_file_id_list(multi_file_id[1]);

    // 发起 RPC 调用
    auto cntl = std::make_unique<brpc::Controller>();
    auto rsp = std::make_unique<bite_im::GetMultiFileRsp>();
    stub.GetMultiFile(cntl.get(), &req, rsp.get(), nullptr);
    
    ASSERT_FALSE(cntl->Failed());
    ASSERT_TRUE(rsp->success());
    
    // 检测返回的 map 中是否包含请求的两个文件 ID
    // 注意：Protobuf 的 map 类型底层是哈希表，可以使用 find() 和 end()
    ASSERT_TRUE(rsp->file_data().find(multi_file_id[0]) != rsp->file_data().end());
    ASSERT_TRUE(rsp->file_data().find(multi_file_id[1]) != rsp->file_data().end());
    
    // 获取返回的数据 map
    auto map = rsp->file_data();
    
    // 根据 ID 从 map 中提取对应的文件数据，并写入本地文件进行验证
    auto file_data1 = map[multi_file_id[0]];
    bite_im::writeFile("base_download_file1",file_data1.file_content());
    
    auto file_data2 = map[multi_file_id[1]];
    bite_im::writeFile("file_download_file2", file_data2.file_content());
}


// ==================== 主函数 ====================
int main(int argc, char *argv[])
{
    // 1. 初始化 gtest 和 gflags 解析命令行参数
    testing::InitGoogleTest(&argc, argv);
    google::ParseCommandLineFlags(&argc, &argv, true);

    // 2. 初始化日志系统（读取传入的模式和文件参数）
    bite_im::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    // 3. 构造微服务发现相关组件
    // 3.1 先构造Rpc信道管理对象（用来存各种服务的连接通道）
    auto sm = std::make_shared<bite_im::ServiceManager>();
    sm->declared(FLAGS_file_service); // 声明我们关心 "文件服务" 的上线/下线状态

    // 3.2 定义当服务上线和下线时的回调函数
    // 当文件服务上线时，触发 onServiceOnline；下线时，触发 onServiceOffline
    auto put_cb = std::bind(&bite_im::ServiceManager::onServiceOnline, sm.get(), std::placeholders::_1, std::placeholders::_2);
    auto del_cb = std::bind(&bite_im::ServiceManager::onServiceOffline, sm.get(), std::placeholders::_1, std::placeholders::_2);
    
    // 3.3 构造服务发现对象，连接到 etcd，监听 /service 目录下文件服务的变化
    bite_im::Discovery::ptr dclient = std::make_shared<bite_im::Discovery>(FLAGS_etcd_host, FLAGS_base_service, put_cb, del_cb);
    
    // 4. 通过 Rpc 信道管理对象，获取提供文件服务的通信信道
    // 注意：这里可能存在时序问题，刚去 etcd 拉取服务列表可能拉不到，所以代码里做了一个重试机制
    channel = sm->choose(FLAGS_file_service);
    if (!channel) {
        // 如果没拿到信道，可能服务还没注册到 etcd 或网络延迟，休眠1秒等待
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return -1; // 这里示例代码直接退出了，严谨的做法应该写个 while 循环重试，或者让 gtest 报错
    }

    // 5. 所有的准备工作和前置检查完毕，开始运行所有 TEST 用例
    return RUN_ALL_TESTS();
}