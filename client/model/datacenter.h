#ifndef DATACENTER_H
#define DATACENTER_H

#include <QWidget>
#include "data.h"
#include "../network/netclient.h"
// 数据调度中心，我们把数据全存在这里，后续通过这里进行调用
namespace model{

class DataCenter : public QObject
{
    Q_OBJECT
public:
    static DataCenter* getInstance();
    ~DataCenter();
private:
    DataCenter();
    static DataCenter* instance;
    //客户端登录服务器对应的登录会话id
    // 为了使会话持久化，就需要一些变量进行记录
    // 如果不进行记录，每次访问服务都需要传密码和账号，那查数据库开销会巨大
    QString loginSessionId = "";
    //当前用户的信息
    UserInfo* myself = nullptr;
    //好友链表
    QList<UserInfo>* friendList = nullptr;
    // 会话链表
    QList<ChatSession>* chatSessionList = nullptr;
    // 记录当前选中的会话是哪个~~
    QString currentChatSessionId = "";
    // 聊天群的成员
    QHash<QString, QList<UserInfo>>* memberList = nullptr;
    // 好友申请列表
    QList<UserInfo>* applyList = nullptr;
    // 不同会话的最新消息列表，记录每个会话的最新消息用于页面展示
    QHash<QString, QList<Message>>* recentMessages = nullptr;
    // 不同会话未读消息的数量，如果不打开就进行累加，打开后就清空
    QHash<QString, int>* unreadMessageCount = nullptr;
    // 好友搜索列表，用于记录搜索的好友结果
    QList<UserInfo>* searchUserResult = nullptr;
    // 历史记录
    QList<Message>* searchMessageResult = nullptr;
    // 聊天验证的键值对，发送申请后会生成两个随机值进行配对，如果配对成功就通过验证
    QString currentVerifyCodeId = "";
    // 持有NetClient
    network::NetClient netClient;

public:
    // 初始化文件
    void initDataFile();
    // 存储数据到文件
    void saveDataFile();
    // 加载文件，用于程序启动时
    void loadDataFile();
    // 清空未读消息数
    void clearUnread(const QString& chatSessionId);
    // 增加未读消息数
    void addUnread(const QString& chatSessionId);
    // 获取未读消息数
    int getUnread(const QString& chatSessionId);
    // 获取当前登录会话id
    const QString& getLoginSessionId() const
    {
        return loginSessionId;
    }
    void ping() { netClient.ping(); }
    // 初始化当前 DataCenter 所持有的 WebSocket 连接
    void initWebsocket();
    // 通过网络获取个人信息
    // 采用异步机制
    void getMyselfAsync();
    UserInfo* getMyself();
    void resetMyself(std::shared_ptr<bite_im::GetUserInfoRsp> resp);

    // 通过网络获取好友列表
    void getFriendListAsync();
    QList<UserInfo>* getFriendList();
    void resetFriendList(std::shared_ptr<bite_im::GetFriendListRsp> resp);
    // 通过网络获取群聊列表
    void getChatSessionListAsync();
    QList<ChatSession>* getChatSessionList();
    void resetChatSessionList(std::shared_ptr<bite_im::GetChatSessionListRsp> resp);

    // 获取单个文件
    void getSingleFileAsync(const QString& fileId);


    // 申请好友列表
    void getApplyListAsync();
    QList<UserInfo>* getApplyList();
    void resetApplyList(std::shared_ptr<bite_im::GetPendingFriendEventListRsp> resp);

    // 获取最近消息列表
    void getRecentMessageListAsync(const QString& chatSessionId, bool updateUi);
    // 获取指定会话已缓存的消息列表
    QList<Message>* getRecentMessageList(const QString& chatSessionId);
    // 保存服务器返回的最近消息
    void resetRecentMessageList(const QString& chatSessionId, std::shared_ptr<bite_im::GetRecentMsgRsp> resp);

    // 获取会话id查询会话信息
    ChatSession* findChatSessionById(const QString& chatSessionId);
    // 根据用户id查询会话信息
    ChatSession* findChatSessionByUserId(const QString& userId);
    // 指定会话置顶
    void topChatSessionInfo(const ChatSession& chatSessionInfo);

    // 设置获取当前选中的会话
    void setCurrentChatSessionId(const QString& chatSessionId);
    const QString& getCurrentChatSessionId();


    // 发送信息给服务器
    void sendTextMessageAsync(const QString& chatSessionId, const QString& cont);
    void sendImageMessageAsync(const QString& chatSessionId, const QByteArray& cont);
    void sendFileMessageAsync(const QString& chatSessionId, const QString& fileName, const QByteArray& cont);

    void sendSpeechMessageAsync(const QString& chatSessionId, const QByteArray& cont);

    // 发送名字进行修改
    void changeUsernameAsync(const QString& username);
    void resetNickname(const QString& username);
    // 修改用户签名
    void changeDescriptionAsync(const QString& desc);
    // 修改签名
    void resetDescription(const QString& desc);
    // 添加消息
    void addMessage(const Message message);
    // 获取短信
    void getVerifyCodeAsync(const QString& phone);
    // 获取验证码
    void resetVerifyCodeId(const QString& verifyCodeId);
    const QString& getVerifyCodeId();
    // 修改电话号码
    void changePhoneAsync(const QString& phone, const QString& verifyCodeId, const QString& verifyCode);
    void resetPhone(const QString& phone);

    void changeAvatarAsync(const QByteArray& imageBytes);
    void resetAvatar(const QByteArray& imageBytes);
    // 根据用户id查询信息
    UserInfo* findFriendById(const QString& userId);
    // 删除好友
    void deleteFriendAsync(const QString& userId);
    void removeFriend(const QString& userId);
    // 添加好友
    void addFriendApplyAsync(const QString& userId);
    // 接受好友申请操作
    void acceptFriendApplyAsync(const QString& userId);
    // 拒绝好友申请操作
    void rejectFriendApplyAsync(const QString& userId);
    UserInfo removeFromApplyList(const QString& userId);

    // 创建群聊
    void createGroupChatSessionAsync(const QList<QString>& userIdList);
    // 获取会话成员列表
    void getMemberListAsync(const QString& chatSessionId);
    void resetMemberList(const QString& chatSessionId, const QList<bite_im::UserInfo>& List);
    QList<UserInfo>* getMemberList(const QString& chatSessionId);

    // 搜索用户
    void searchUserAsync(const QString& searchKey);
    void resetSearchUserResult(const QList<bite_im::UserInfo>& userList);
    QList<UserInfo>* getSearchUserResult();

    void searchMessageAsync(const QString& searchKey);
    void resetSearchMessageResult(const QList<bite_im::MessageInfo> &msgList);
    QList<Message>* getSearchMessageResult();

    void searchMessageByTimeAsync(const QDateTime& begTime, const QDateTime& endTime);
    // 请求网络进行登录
    void userLoginAsync(const QString& username, const QString& password);
    // 设置登录
    void resetLoginSessionId(const QString& loginSessionId);
    // 注册
    void userRegisterAsync(const QString& username, const QString& password);
    // 手机登录与注册
    void phoneLoginAsync(const QString& phone, const QString& verifyCode);
    void phoneRegisterAsync(const QString& phone, const QString& verifyCode);

    void speechConvertTextAsync(const QString& fileId, const QByteArray& cont);
signals:
    void getMyselfDone();
    void getFriendListDone();
    void getChatSessionListDone();
    void getApplyListDone();
    void getRecentMessageListDone(const QString& chatSessionId);
    void sendMessageDone(const QString& chatSessionId, MessageType messageType,
                         const QByteArray& cont, const QString& extraInfo);
    void getRecentMessageListDoneNoUI(const QString& chatSessionId);
    void updataLastMessage(const QString& chatSessionId);
    void receiveMessageDone(const Message& lastMessage);
    void changeNicknameDone();
    void changeDescriptionDone();
    void getVerifyCodeDone();
    void changePhoneDone();
    void changeAvatarDone();
    void deleteFriendDone();
    void clearCurrentSession();
    void addFriendApplyDone();
    void receiveFriendApplyDone();
    void acceptFriendApplyDone();
    void rejectFriendApplyDone();
    void receiveFriendProcessDone(const QString& nickname, bool agree);
    void createGroupChatSessionDone();
    void receiveSessionCreateDone();
    void getMemberListDone(const QString& chatSessionId);
    void searchUserDone();
    void searchMessageDone();
    void userLoginDone(bool ok, const QString& reason);
    void userRegisterDone(bool ok, const QString& reason);
    void phoneLoginDone(bool ok, const QString& reason);
    void phoneRegisterDone(bool ok, const QString& reason);
    void getSingleFileDone(const QString& fileId, const QByteArray& fileContent);
    void speechConvertTextDone(const QString& fileId, const QString& text);
};
}

#endif // DATACENTER_H
