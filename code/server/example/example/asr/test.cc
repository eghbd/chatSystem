#include "../../common/asr.hpp"
#include "gflags/gflags.h"
#include <cstdlib>

// 凭据不写死默认值：优先命令行传参，其次读环境变量
DEFINE_string(app_id, "", "语音平台应用ID（默认读环境变量 BAIDU_APP_ID）");
DEFINE_string(api_key, "", "语音平台API密钥（默认读环境变量 BAIDU_API_KEY）");
DEFINE_string(secret_key, "", "语音平台加密密钥（默认读环境变量 BAIDU_SECRET_KEY）");

DEFINE_bool(run_mode, false, "程序的运行模式，false-调试； true-发布；");
DEFINE_string(log_file, "", "发布模式下，用于指定日志的输出文件");
DEFINE_int32(log_level, 0, "发布模式下，用于指定日志输出等级");

int main(int argc, char *argv[])
{
    google::ParseCommandLineFlags(&argc, &argv, true);
    init_logger(FLAGS_run_mode, FLAGS_log_file, FLAGS_log_level);

    std::string app_id = FLAGS_app_id;
    std::string api_key = FLAGS_api_key;
    std::string secret_key = FLAGS_secret_key;
    if (app_id.empty())     { const char *p = getenv("BAIDU_APP_ID");   if (p) app_id = p; }
    if (api_key.empty())    { const char *p = getenv("BAIDU_API_KEY");  if (p) api_key = p; }
    if (secret_key.empty()) { const char *p = getenv("BAIDU_SECRET_KEY"); if (p) secret_key = p; }
    if (app_id.empty() || api_key.empty() || secret_key.empty()) {
        std::cerr << "请通过命令行参数或环境变量提供百度语音平台凭据" << std::endl;
        return -1;
    }

    ASRClient client(app_id, api_key, secret_key);

    std::string file_content;
    aip::get_file_content("16k.pcm", &file_content);

    std::string res = client.recognize(file_content);
    if (res.empty()) {
        return -1;
    }
    std::cout << res << std::endl;
    return 0;
}