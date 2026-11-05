#pragma once
#include "aip-cpp-sdk/speech.h"
#include "logger.hpp"

namespace bite_im {

class ASRClient {
public:
    using ptr = std::shared_ptr<ASRClient>;

    // 构造函数：初始化百度云语音识别客户端
    // 需要传入在百度云控制台申请的 应用AppID、API Key 和 Secret Key
    ASRClient(const std::string &app_id,
              const std::string &api_key,
              const std::string &secret_key)
        : _client(app_id, api_key, secret_key) {}

    /* ============================================================
     * 核心接口：将音频数据识别为文字
     * @param speech_data: 二进制音频数据流
     * @param err: 传出参数，若识别失败，带回错误信息
     * @return: 识别成功的文字结果；失败返回空串
     * ============================================================ */
    std::string recognize(const std::string &speech_data, std::string &err) {
        // 调用百度云 SDK 的 recognize 接口发起 HTTP 请求
        // 参数
        // 1. speech_data: 音频数据
        // 2. "pcm"      : 音频格式（PCM 是未压缩的原始音频格式）
        // 3. 16000      : 采样率（16000 Hz 是语音识别的标准采样率）
        // 4. aip::null  : 可选参数字典（这里不需要额外参数，传空）
        Json::Value result = _client.recognize(speech_data, "pcm", 16000, aip::null);

        // 判断结果：百度 SDK 返回的 JSON 中，"err_no" 为 0 表示成功
        if (result["err_no"].asInt() != 0) {
            // 失败：提取错误信息，记日志，并赋给出参 err
            LOG_ERROR("语音识别失败：{}", result["err_msg"].asString());
            err = result["err_msg"].asString();
            return std::string(); // 返回空串表示失败
        }

        // 成功：返回识别出的文本数组中的第一个结果
        // result["result"] 是一个 JSON 数组，例如 ["今天天气不错"]
        return result["result"][0].asString();
    }

private:
    aip::Speech _client; // 百度云语音识别底层客户端对象
};

} // namespace bite_im