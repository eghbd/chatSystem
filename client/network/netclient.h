#ifndef NETCLIENT_H
#define NETCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QWebSocket>
#include <QProtobufSerializer>
#include <QNetworkReply>
#include "../model/data.h"

// 使用前置声明包含头文件
namespace model{
    class DataCenter;
}
namespace network{
class NetClient : public QObject
{
private:
    const QString HTTP_URL = "http://192.168.194.129:10008";
    const QString WEBSOCKET_URL = "ws://192.168.194.129:10007/ws";
    Q_OBJECT
public:
    NetClient(model::DataCenter* dataCenter);

    // 通过 HTTP GET 请求 `/ping` 接口，异步探测服务器是否在线，并把响应内容打印到日志
    void ping();

    // 初始化websocket
    void initWebsocket();

    void handleWsResponse(const bite_im::NotifyMessage& notifyMessage);

    void handleWsMessage(const Message& message);
    void handleWsAddFriendApply(const UserInfo& userId);
    void handleWsSessionCreate(const ChatSession& chatSessionInfo);
    // 身份认证
    void sendAuth();

    // 生成请求 id
    static QString makeRequestId();

    // 发送请求的逻辑封装，因为很多情况下都会先判断链接情况
    QNetworkReply* sendHttpRequest(const QString& path, const QByteArray& body);

    // 处理响应的函数(内容有http准确性，反序列化，业务正确性)
    // 因为请求不同，所以我们得到的反序列化对象结构可能不同，所以我们使用模板进行操作
    // 这里有用到共享指针概念，因为如果返回类型T，拷贝时开销会比较大
    template<typename T>
    std::shared_ptr<T> handleHttpResponse(QNetworkReply* httpResp, bool* ok, QString* reason)
    {
        // 记得每次释放httpResp
        // 先判断http链接是否正常
        if(httpResp->error() != QNetworkReply::NoError)
        {
            *ok = false;
            *reason = httpResp->errorString();
            httpResp->deleteLater();
            return std::shared_ptr<T>();
        }
        // 获取http的body
        QByteArray respBody = httpResp->readAll();
        // 反序列化
        std::shared_ptr<T> respObj = std::make_shared<T>();
        respObj->deserialize(&serializer, respBody);
        // 判断正确性
        if(!respObj->success())
        {
            *ok = false;
            *reason = respObj->errmsg();
            httpResp->deleteLater();
            return std::shared_ptr<T>();
        }
        httpResp->deleteLater();
        *ok = true;
        return respObj;
    }

    //
    void getMyself(const QString& loginSessionId);

    void getFriendList(const QString& loginSessionId);

    void getChatSessionList(const QString& loginSessionId);

    void getApplyList(const QString& loginSessionId);

    void getRecentMessageList(const QString& loginSessionId, const QString& chatSessionId, bool updateUi);

    void sendMessage(const QString& loginSessionId, const QString& chatSessionId, MessageType messageType, const QByteArray& cont, const QString& extraInfo);

    void receiveMessage(const QString& chatSessionId);

    void changeUsername(const QString& loginSessionId, const QString& username);

    void changeDescription(const QString& loginSessionId, const QString& desc);

    void getVerifyCode(const QString& phone);

    void changePhone(const QString& loginSessionId, const QString& phone, const QString& verifyCodeId, const QString& verifyCode);

    void changeAvatar(const QString& loginSessionId, const QByteArray& imageBytes);

    void deleteFriend(const QString& loginSessionId, const QString& userId);

    void handleWsRemoveFriend(const QString& userId);

    void addFriendApply(const QString& loginSessionId, const QString& userId);

    void acceptFriendApplyAsync(const QString& userId);

    void acceptFriendApply(const QString& loginSessionId, const QString& userId);

    void rejectFriendApply(const QString& loginSessionId, const QString& userId);

    void createGroupChatSession(const QString& loginSessionId, const QList<QString>& userIdList);
    void getMemberList(const QString& loginSessionId, const QString& chatSessionId);
    void searchUser(const QString& loginSessionId, const QString &searchKey);
    void searchMessage(const QString& loginSessionId, const QString& chatSessionId, const QString& searchKey);
    void searchMessageByTime(const QString& loginSessionId, const QString& chatSessionId, const QDateTime& begTime, const QDateTime& endTime);

    void userLogin(const QString& username, const QString& password);

    void userRegister(const QString& username, const QString& password);
    void phoneLogin(const QString& phone, const QString& verifyCodeId, const QString& verifyCode);
    void phoneRegister(const QString& phone, const QString& verifyCodeId, const QString& verifyCode);

    void getSingleFile(const QString& loginSessionId, const QString& fileId);

    void speechConvertText(const QString& loginSessionId, const QString& fileId, const QByteArray& cont);









private:
    model::DataCenter* dataCenter;

    // http客户端
    QNetworkAccessManager httpClient;

    // web客户端
    QWebSocket websocketClient;

    bool websocketInitialized = false;

    // 序列化器,用于对象和字节相互转换的
    QProtobufSerializer serializer;


    void handleWsAddFriendProcess(const UserInfo& userInfo, bool agree);
signals:
};
}
#endif // NETCLIENT_H
