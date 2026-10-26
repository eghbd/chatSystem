#pragma once
// 头文件防重包含：同一编译单元只展开一次，避免重复定义

#include <cstdlib>
#include <iostream>
#include <memory>
// 阿里云核心 SDK 头文件
#include <alibabacloud/core/AlibabaCloud.h>       // InitializeSdk / ShutdownSdk / ClientConfiguration / Credentials
#include <alibabacloud/core/CommonRequest.h>      // 通用请求对象（不依赖具体产品 SDK，按 RPC 参数拼装）
#include <alibabacloud/core/CommonClient.h>       // 通用客户端，可调用任意阿里云产品的 OpenAPI
#include <alibabacloud/core/CommonResponse.h>     // 通用响应对象（isSuccess / error）
#include "logger.hpp"                             // 日志封装

namespace bite_im {

// ======================= 阿里云短信（DMS）客户端 =======================
// 职责：封装阿里云短信服务的 SendSms 接口，向指定手机号发送验证码。
// 设计要点：
//   1. 用 CommonClient + CommonRequest 走通用 RPC 调用，不需要引入 dysmsapi 专用 SDK
//   2. 构造时 InitializeSdk，析构时 ShutdownSdk，成对管理 SDK 生命周期
//   3. 通过 shared_ptr 别名 ptr 方便上层持有
class DMSClient {
public:
    using ptr = std::shared_ptr<DMSClient>;

    // 构造：初始化阿里云 SDK，创建通用客户端
    // access_key_id / access_key_secret：阿里云账号的访问密钥（需有短信发送权限）
    DMSClient(const std::string &access_key_id,
              const std::string &access_key_secret) {
        AlibabaCloud::InitializeSdk();   // 全局初始化 SDK（进程内只需一次，这里按对象构造调用）

        // 客户端配置：地域 cn-chengdu，连接/读取超时分别 1.5s / 4s
        AlibabaCloud::ClientConfiguration configuration("cn-chengdu");
        configuration.setConnectTimeout(1500);
        configuration.setReadTimeout(4000);

        // 凭证：用 AK/SK 构造
        AlibabaCloud::Credentials credential(access_key_id, access_key_secret);

        // 创建通用客户端（unique_ptr 独占持有）
        _client = std::make_unique<AlibabaCloud::CommonClient>(credential, configuration);
    }

    // 析构：关闭 SDK，释放全局资源
    ~DMSClient() { AlibabaCloud::ShutdownSdk(); }

    // 发送短信验证码
    // phone：目标手机号；code：验证码内容
    // 返回：true 发送成功，false 失败（失败原因写日志）
    bool send(const std::string &phone, const std::string &code) {
        // 1. 构造 RPC 风格的通用请求
        AlibabaCloud::CommonRequest request(AlibabaCloud::CommonRequest::RequestPattern::RpcPattern);
        request.setHttpMethod(AlibabaCloud::HttpRequest::Method::Post);

        // 2. 指定短信服务的 OpenAPI 元信息
        request.setDomain("dysmsapi.aliyuncs.com");  // 短信服务域名
        request.setVersion("2017-05-25");            // 短信服务 API 版本

        // 3. 设置 SendSms 接口的业务参数
        request.setQueryParameter("Action", "SendSms");            // 调用的接口名
        request.setQueryParameter("SignName", "bitejiuyeke");      // 短信签名（需在阿里云控制台审核通过）
        request.setQueryParameter("TemplateCode", "SMS_465324787");// 短信模板编号（需审核通过）
        request.setQueryParameter("PhoneNumbers", phone);          // 接收手机号

        // 4. 模板变量：模板里占位符 ${code} 用 JSON 传入
        std::string param_code = "{\"code\":\"" + code + "\"}";
        request.setQueryParameter("TemplateParam", param_code);

        // 5. 发起调用并检查结果
        auto response = _client->commonResponse(request);
        if (!response.isSuccess()) {
            LOG_ERROR("短信验证码请求失败：{}", response.error().errorMessage());
            return false;
        }
        return true;
    }

private:
    std::unique_ptr<AlibabaCloud::CommonClient> _client;  // 阿里云通用客户端
};

} // namespace bite_im