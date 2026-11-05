// 主要实现用户管理子服务的服务器的搭建（注：原注释写的语音识别是错的）
#include "user_server.hpp"

/* ============================================================
 * 1. 使用 gflags 定义所有的启动参数
 * 这样可以通过命令行或配置文件动态传入，而无需重新编译代码。
 * ============================================================ */
DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

// 服务注册与发现相关参数
DEFINE_string(registry_host, "http://127.0.0.1:2379", "服务注册中心地址 (etcd)");
DEFINE_string(instance_name, "/user_service/instance", "当前实例在 etcd 中的节点名称");
DEFINE_string(access_host, "127.0.0.1:10003", "当前实例对外提供服务的地址(IP:端口)");

// RPC 服务器参数
DEFINE_int32(listen_port, 10003, "Rpc服务器监听端口");
DEFINE_int32(rpc_timeout, -1, "Rpc调用超时时间");
DEFINE_int32(rpc_threads, 1, "Rpc的IO线程数量");

// 服务依赖配置
DEFINE_string(base_service, "/service", "服务监控根目录");
DEFINE_string(file_service, "/service/file_service", "文件管理子服务名称 (用于服务发现)");

// 外部中间件连接配置
DEFINE_string(es_host, "http://127.0.0.1:9200/", "ES搜索引擎服务器URL");
DEFINE_string(mysql_host, "127.0.0.1", "Mysql服务器访问地址");
DEFINE_string(mysql_user, "root", "Mysql服务器访问用户名");
DEFINE_string(mysql_pswd, "123456", "Mysql服务器访问密码");
DEFINE_string(mysql_db, "bite_im", "Mysql默认库名称");
DEFINE_string(mysql_cset, "utf8", "Mysql客户端字符集");
DEFINE_int32(mysql_port, 0, "Mysql服务器访问端口");
DEFINE_int32(mysql_pool_count, 4, "Mysql连接池最大连接数量");

DEFINE_string(redis_host, "127.0.0.1", "Redis服务器访问地址");
DEFINE_int32(redis_port, 6379, "Redis服务器访问端口");
DEFINE_int32(redis_db, 0, "Redis默认库号");
DEFINE_bool(redis_keep_alive, true, "Redis长连接保活选项");

// 短信平台密钥（模拟模式下置空，接入真实短信时填回）
DEFINE_string(dms_key_id, "", "短信平台密钥ID");
DEFINE_string(dms_key_secret, "", "短信平台密钥");


/* ============================================================
 * 2. 程序入口
 * ============================================================ */
int main(int argc, char *argv[])
{
    // 1. 解析命令行参数（必须在所有 FLAGS_xxx 使用前调用）
    google::ParseCommandLineFlags(&argc, &argv, true);

    // 2. 初始化日志系统（调试模式打终端，发布模式写文件）
    bite_im::init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    // 3. 使用 Builder 模式，分步构建并注入所有依赖
    bite_im::UserServerBuilder usb;
    
    // 模拟模式停用，如果没有真实短信平台账号，就不初始化 DMS 模块
    // usb.make_dms_object(FLAGS_dms_key_id, FLAGS_dms_key_secret);

    // 3.1 初始化 ES 搜索引擎（用于用户搜索）
    usb.make_es_object({FLAGS_es_host});

    // 3.2 初始化 MySQL 连接池（用于持久化用户数据）
    usb.make_mysql_object(FLAGS_mysql_user, FLAGS_mysql_pswd, FLAGS_mysql_host, 
        FLAGS_mysql_db, FLAGS_mysql_cset, FLAGS_mysql_port, FLAGS_mysql_pool_count);

    // 3.3 初始化 Redis（用于存储验证码、会话状态）
    usb.make_redis_object(FLAGS_redis_host, FLAGS_redis_port, FLAGS_redis_db, FLAGS_redis_keep_alive);

    // 3.4 初始化服务发现模块（监听 etcd 中的 file_service，为后续调用文件服务做准备）
    usb.make_discovery_object(FLAGS_registry_host, FLAGS_base_service, FLAGS_file_service);

    // 3.5 初始化并启动 RPC 服务器（监听 10003 端口）
    usb.make_rpc_server(FLAGS_listen_port, FLAGS_rpc_timeout, FLAGS_rpc_threads);

    // 3.6 将当前用户服务注册到 etcd（让网关和其他服务能找到自己）
    usb.make_registry_object(FLAGS_registry_host, FLAGS_base_service + FLAGS_instance_name, FLAGS_access_host);

    // 4. 完成组装，获取最终服务器对象
    auto server = usb.build();

    // 5. 启动服务（阻塞式运行，进入事件循环）
    server->start();
    
    return 0;
}