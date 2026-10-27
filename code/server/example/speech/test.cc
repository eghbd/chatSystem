#include "aip-cpp-sdk/speech.h"
#include <cstdlib>
#include <iostream>

void asr(aip::Speech &client)
{
    std::string file_content;
    aip::get_file_content("16k.pcm", &file_content);

    Json::Value result = client.recognize(file_content, "pcm", 16000, aip::null);
    if (result["err_no"].asInt() != 0) {
        std::cout << result["err_msg"].asString() << std::endl;
        return;
    }

    std::cout << result["result"][0].asString() << std::endl;
}

int main()
{
    // 设置APPID/AK/SK：从环境变量读取，不写进代码
    const char *app_id_env = getenv("BAIDU_APP_ID");
    const char *api_key_env = getenv("BAIDU_API_KEY");
    const char *secret_key_env = getenv("BAIDU_SECRET_KEY");
    if (app_id_env == nullptr || api_key_env == nullptr || secret_key_env == nullptr) {
        std::cerr << "请先设置环境变量 BAIDU_APP_ID / BAIDU_API_KEY / BAIDU_SECRET_KEY" << std::endl;
        return -1;
    }
    std::string app_id = app_id_env;
    std::string api_key = api_key_env;
    std::string secret_key = secret_key_env;

    aip::Speech client(app_id, api_key, secret_key);

    asr(client);
    return 0;
}