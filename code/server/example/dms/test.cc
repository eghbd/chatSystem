#include <cstdlib>
#include <iostream>
#include <alibabacloud/core/AlibabaCloud.h>
#include <alibabacloud/core/CommonRequest.h>
#include <alibabacloud/core/CommonClient.h>
#include <alibabacloud/core/CommonResponse.h>

using namespace std;
using namespace AlibabaCloud;

int main( int argc, char** argv )
{
    // 凭据不写进代码：从环境变量读取，避免被提交进仓库
    const char *ak_id_env = getenv("ALIYUN_ACCESS_KEY_ID");
    const char *ak_secret_env = getenv("ALIYUN_ACCESS_KEY_SECRET");
    if (ak_id_env == nullptr || ak_secret_env == nullptr) {
        std::cerr << "请先设置环境变量 ALIYUN_ACCESS_KEY_ID / ALIYUN_ACCESS_KEY_SECRET" << std::endl;
        return -1;
    }

    AlibabaCloud::InitializeSdk();
    AlibabaCloud::ClientConfiguration configuration( "cn-chengdu" );
    // specify timeout when create client.
    configuration.setConnectTimeout(1500);
    configuration.setReadTimeout(4000);
    std::string access_key_id = ak_id_env;
    std::string access_key_secret = ak_secret_env;
    AlibabaCloud::Credentials credential(access_key_id, access_key_secret);
    /* use STS Token
    credential.setSessionToken( getenv("ALIBABA_CLOUD_SECURITY_TOKEN") );
    */
    AlibabaCloud::CommonClient client( credential, configuration );
    AlibabaCloud::CommonRequest request(AlibabaCloud::CommonRequest::RequestPattern::RpcPattern);
    request.setHttpMethod(AlibabaCloud::HttpRequest::Method::Post);
    request.setDomain("dysmsapi.aliyuncs.com");
    request.setVersion("2017-05-25");
    request.setQueryParameter("Action", "SendSms");
    request.setQueryParameter("SignName", "bitejiuyeke");
    request.setQueryParameter("TemplateCode", "SMS_465324787");
    // 收信手机号同样不写死，指向环境变量
    const char *phone = getenv("ALIYUN_SMS_TEST_PHONE");
    request.setQueryParameter("PhoneNumbers", phone ? phone : "");
    request.setQueryParameter("TemplateParam", "{\"code\":\"1234\"}");

    auto response = client.commonResponse(request);
    if (response.isSuccess()) {
        printf("request success.\n");
        printf("result: %s\n", response.result().payload().c_str());
    } else {
        printf("error: %s\n", response.error().errorMessage().c_str());
        printf("request id: %s\n", response.error().requestId().c_str());
    }

    AlibabaCloud::ShutdownSdk();
    return 0;
}