#include "../../common/dms.hpp"
#include "gflags/gflags.h"
#include <cstdlib>

// 凭据不写死默认值：优先命令行传参，其次读环境变量
DEFINE_string(key_id, "", "平台访问密钥ID（默认读环境变量 ALIYUN_ACCESS_KEY_ID）");
DEFINE_string(key_secret, "", "平台访问密钥（默认读环境变量 ALIYUN_ACCESS_KEY_SECRET）");

DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    std::string key_id = FLAGS_key_id;
    std::string key_secret = FLAGS_key_secret;
    if (key_id.empty()) {
        const char *p = getenv("ALIYUN_ACCESS_KEY_ID");
        if (p) key_id = p;
    }
    if (key_secret.empty()) {
        const char *p = getenv("ALIYUN_ACCESS_KEY_SECRET");
        if (p) key_secret = p;
    }
    if (key_id.empty() || key_secret.empty()) {
        std::cerr << "请通过 --key_id/--key_secret 或环境变量提供阿里云访问密钥" << std::endl;
        return -1;
    }

    DMSClient client(key_id, key_secret);
    const char *phone = getenv("ALIYUN_SMS_TEST_PHONE");
    client.send(phone ? phone : "", "5678");

    return 0;
}