#include "datacenter.h"
#include <QDir>
#include <QStandardPaths>
#include <QJsonObject>
#include <QJsonDocument>
namespace model{

DataCenter* DataCenter::instance = nullptr;

DataCenter *DataCenter::getInstance()
{
    if(instance == nullptr)
    {
        instance = new DataCenter();
    }
    return instance;
}

DataCenter::~DataCenter()
{
    // 释放所有成员变量
    delete myself;
    delete friendList;
    delete chatSessionList;
    delete memberList;
    delete applyList;
    delete recentMessages;
    delete unreadMessageCount;
    delete searchUserResult;
    delete searchMessageResult;
}

DataCenter::DataCenter() : netClient (this)
{
    // 此处只是把这几个 hash 类型的属性 new 出实例. 其他的 QList 类型的属性, 都暂时不实例化.
    // 主要是为了使用 nullptr 表示 "非法状态"
    // 对于 hash 来说, 不关心整个 QHash 是否是 nullptr, 而是关心, 某个 key 对应的 value 是否存在~~
    // 通过 key 是否存在, 也能表示该值是否有效.
    recentMessages = new QHash<QString, QList<Message>>();
    memberList = new QHash<QString, QList<UserInfo>>();
    unreadMessageCount = new QHash<QString, int>();

    // 加载数据
    loadDataFile();
}
/////////////
/// 临时借用一下本地存储
void DataCenter::initDataFile()
{
    // 构造出文件的路径, 使用 appData 存储文件
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString filePath = basePath + "/ChatClient.json";
    LOG << "filePath=" << filePath;

    QDir dir;
    if (!dir.exists(basePath)) {
        dir.mkpath(basePath);
    }

    // 构造好文件路径之后, 把文件创建出来.
    // 写方式打开, 并且写入初始内容
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG << "打开文件失败!" << file.errorString();
        return;
    }
    // 打开成功, 写入初始内容.
    QString data = "{\n\n}";
    file.write(data.toUtf8());
    file.close();
}

void DataCenter::saveDataFile()
{
    QString filePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ChatClient.json";
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        LOG << "文件打开失败" << file.errorString();
        return;
    }
    QJsonObject jsonObj;
    jsonObj["loginSessionId"] = loginSessionId;
    QJsonObject jsonUnread;
    for(auto it = unreadMessageCount->begin();it != unreadMessageCount->end();it++)
    {
        jsonUnread[it.key()] = it.value();
    }
    jsonObj["unread"] = jsonUnread;
    QJsonDocument jsonDoc(jsonObj);
    QString s = jsonDoc.toJson();
    file.write(s.toUtf8());
    file.close();
}

void DataCenter::loadDataFile()
{
    // 确保在加载之前, 先针对文件进行初始化操作.
    QString filePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/ChatClient.json";

    // 判定文件是否存在, 不存在则初始化, 并创建出新的空白的 json 文件
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        initDataFile();
    }

    // 读方式打开文件
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG << "打开文件失败! " << file.errorString();
        return;
    }

    // 读取到文件内容, 解析为 JSON 对象
    QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll());
    if (jsonDoc.isNull()) {
        LOG << "解析 JSON 文件失败! JSON 文件格式有错误!";
        file.close();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    this->loginSessionId = jsonObj["loginSessionId"].toString();
    LOG << "loginSessionId=" << this->loginSessionId;

    this->unreadMessageCount->clear();
    QJsonObject jsonUnread = jsonObj["unread"].toObject();
    for (auto it = jsonUnread.begin(); it != jsonUnread.end(); ++it) {
        this->unreadMessageCount->insert(it.key(), it.value().toInt());
    }

    file.close();
}

void DataCenter::clearUnread(const QString &chatSessionId)
{
    (*unreadMessageCount)[chatSessionId] = 0;
    saveDataFile();
}

void DataCenter::addUnread(const QString &chatSessionId)
{
    (*unreadMessageCount)[chatSessionId]++;
    saveDataFile();
}

int DataCenter::getUnread(const QString &chatSessionId)
{
    return (*unreadMessageCount)[chatSessionId];
}

void DataCenter::getMyselfAsync()
{
    netClient.getMyself(loginSessionId);
}

UserInfo *DataCenter::getMyself()
{
    return myself;
}

QList<UserInfo> *DataCenter::getFriendList()
{
    return friendList;
}

void DataCenter::resetFriendList(std::shared_ptr<bite_im::GetFriendListRsp> resp)
{
    if (friendList == nullptr) {
        friendList = new QList<UserInfo>();
    }
    friendList->clear();

    const QList<bite_im::UserInfo>& friendListPB = resp->friendList();
    for (auto& f : friendListPB) {
        UserInfo userInfo;
        userInfo.load(f);
        friendList->push_back(userInfo);
    }
}

void DataCenter::getChatSessionListAsync()
{
    netClient.getChatSessionList(loginSessionId);
}

QList<ChatSession> *DataCenter::getChatSessionList()
{
    return chatSessionList;
}

void DataCenter::resetChatSessionList(std::shared_ptr<bite_im::GetChatSessionListRsp> resp)
{
    if (chatSessionList == nullptr) {
        chatSessionList = new QList<ChatSession>();
    }
    chatSessionList->clear();

    auto& chatSessionListPB = resp->chatSessionInfoList();
    for (auto& c : chatSessionListPB) {
        ChatSession chatSession;
        chatSession.load(c);
        chatSessionList->push_back(chatSession);
    }
}

void DataCenter::getSingleFileAsync(const QString &fileId)
{
    netClient.getSingleFile(loginSessionId, fileId);
}

void DataCenter::getApplyListAsync()
{
    netClient.getApplyList(loginSessionId);
}

QList<UserInfo> *DataCenter::getApplyList()
{
    return applyList;
}

void DataCenter::resetApplyList(std::shared_ptr<bite_im::GetPendingFriendEventListRsp> resp)
{
    if (applyList == nullptr) {
        applyList = new QList<UserInfo>();
    }
    applyList->clear();

    auto& eventList = resp->event();
    for (auto& event : eventList) {
        UserInfo userInfo;
        userInfo.load(event.sender());
        applyList->push_back(userInfo);
    }
}

void DataCenter::getRecentMessageListAsync(const QString &chatSessionId, bool updateUi)
{
    netClient.getRecentMessageList(loginSessionId, chatSessionId, updateUi);
}

QList<Message> *DataCenter::getRecentMessageList(const QString &chatSessionId)
{
    if(!recentMessages->contains(chatSessionId)){
        return nullptr;
    }
    return &(*recentMessages)[chatSessionId];
}

void DataCenter::resetRecentMessageList(const QString &chatSessionId, std::shared_ptr<bite_im::GetRecentMsgRsp> resp)
{
    // 这里需要引用类型，才能修改哈希表内部内容
    QList<Message>& messageList = (*recentMessages)[chatSessionId];
    messageList.clear();
    for(auto& m : resp->msgList())
    {
        Message message;
        message.load(m);
        messageList.append(message);
    }
}

ChatSession *DataCenter::findChatSessionById(const QString &chatSessionId)
{
    if(chatSessionList == nullptr)
    {
        return nullptr;
    }
    for(auto& i : *chatSessionList)
    {
        if(i.sessionId == chatSessionId)
        {
            return &i;
        }
    }
    return nullptr;
}

ChatSession *DataCenter::findChatSessionByUserId(const QString &userId)
{
    if(chatSessionList == nullptr)
    {
        return nullptr;
    }
    for(auto& i : *chatSessionList)
    {
        if(i.userId == userId)
        {
            return &i;
        }
    }
    return nullptr;
}

void DataCenter::topChatSessionInfo(const ChatSession &chatSessionInfo)
{
    if(chatSessionList == nullptr)
    {
        return;
    }
    auto it = chatSessionList->begin();
    for(; it != chatSessionList->end();it++)
    {
        if(it -> sessionId == chatSessionInfo.sessionId)break;
    }
    if(it == chatSessionList->end())
    {
        return;
    }
    // 备份，然后删除
    ChatSession backup = chatSessionInfo;
    chatSessionList->erase(it);
    // 把备份元素头插
    chatSessionList->push_front(backup);
}

void DataCenter::setCurrentChatSessionId(const QString &chatSessionId)
{
    this->currentChatSessionId = chatSessionId;
}

const QString &DataCenter::getCurrentChatSessionId()
{
    return this->currentChatSessionId;
}

void DataCenter::sendTextMessageAsync(const QString &chatSessionId, const QString &cont)
{
    netClient.sendMessage(loginSessionId, chatSessionId, MessageType::TEXT_TYPE, cont.toUtf8(), "");
}

void DataCenter::sendFileMessageAsync(const QString &chatSessionId, const QString &fileName, const QByteArray &cont)
{
    netClient.sendMessage(loginSessionId, chatSessionId, MessageType::FILE_TYPE, cont, fileName);
}

void DataCenter::sendSpeechMessageAsync(const QString& chatSessionId, const QByteArray& cont)
{
    netClient.sendMessage(loginSessionId, chatSessionId, MessageType::SPEECH_TYPE, cont, "");
}

void DataCenter::sendImageMessageAsync(const QString &chatSessionId, const QByteArray &cont)
{
    netClient.sendMessage(loginSessionId, chatSessionId, MessageType::IMAGE_TYPE, cont, "");
}

void DataCenter::initWebsocket()
{
    netClient.initWebsocket();
}

void DataCenter::changeUsernameAsync(const QString &username)
{
    netClient.changeUsername(loginSessionId, username);
}

void DataCenter::resetNickname(const QString &username)
{
    myself->userName = username;
}

void DataCenter::changeDescriptionAsync(const QString &desc)
{
    netClient.changeDescription(loginSessionId, desc);
}

void DataCenter::resetDescription(const QString &desc)
{
    if (myself == nullptr) {
        return;
    }
    myself->des = desc;
}

void DataCenter::addMessage(const Message message)
{
    QList<Message>& messageList = (*recentMessages)[message.sessionId];
    messageList.push_back(message);
}

void DataCenter::getVerifyCodeAsync(const QString &phone)
{
    netClient.getVerifyCode(phone);
}

void DataCenter::resetVerifyCodeId(const QString &verifyCodeId)
{
    this->currentVerifyCodeId = verifyCodeId;
}

const QString &DataCenter::getVerifyCodeId()
{
    return currentVerifyCodeId;
}

void DataCenter::changePhoneAsync(const QString &phone, const QString &verifyCodeId, const QString &verifyCode)
{
    netClient.changePhone(loginSessionId, phone, verifyCodeId, verifyCode);
}

void DataCenter::resetPhone(const QString &phone)
{
    if (myself == nullptr) {
        return;
    }
    myself->phone = phone;
}

void DataCenter::changeAvatarAsync(const QByteArray &imageBytes)
{
    netClient.changeAvatar(loginSessionId, imageBytes);
}

void DataCenter::resetAvatar(const QByteArray &imageBytes)
{
    if (myself == nullptr) {
        return;
    }
    myself->avatar = Utils::makeIcon(imageBytes);
}

UserInfo* DataCenter::findFriendById(const QString &userId)
{
    if(this->friendList == nullptr)
    {
        return nullptr;
    }
    for(auto& f : *friendList)
    {
        if(f.userId == userId)
        {
            return &f;
        }
    }
    return nullptr;
}

void DataCenter::deleteFriendAsync(const QString &userId)
{
    netClient.deleteFriend(loginSessionId, userId);
}

void DataCenter::removeFriend(const QString &userId)
{
    // 遍历 friendList, 删除其中匹配的元素即可.
    if (friendList == nullptr || chatSessionList == nullptr) {
        return;
    }
    friendList->removeIf([=](const UserInfo& userInfo) {
        // 返回 true 要删除的元素. false 直接跳过不删除.
        return userInfo.userId == userId;
    });

    // 还要考虑会话列表.
    // 没有好友, 保留会话, 后续往会话里发消息啥的, 就都不好处理了.
    // 删除会话操作, 客户端和服务器分别都会删除.
    chatSessionList->removeIf([=](const ChatSession& chatSessionInfo) {
        if (chatSessionInfo.userId == "") {
            // 群聊, 不受影响
            return false;
        }
        if (chatSessionInfo.userId == userId) {
            // 当前这个会话要删除了, 并且要删除的会话又是选中的会话, 才真正清空当前会话
            // 此处如果删除的会话, 正好是用户正在选中的会话, 此时就需要把当前选中会话的内容(标题和消息列表)都清空
            if (chatSessionInfo.sessionId == this->currentChatSessionId) {
                emit this->clearCurrentSession();
            }
            return true;
        }
        return false;
    });
}

void DataCenter::addFriendApplyAsync(const QString &userId)
{
    netClient.addFriendApply(loginSessionId, userId);
}

void DataCenter::acceptFriendApplyAsync(const QString &userId)
{
    netClient.acceptFriendApply(loginSessionId, userId);
}

void DataCenter::rejectFriendApplyAsync(const QString &userId)
{
    netClient.rejectFriendApply(loginSessionId, userId);
}

UserInfo DataCenter::removeFromApplyList(const QString &userId)
{
    if (applyList == nullptr) {
        return UserInfo();
    }

    for (auto it = applyList->begin(); it != applyList->end(); ++it) {
        if (it->userId == userId) {
            // 复制以下这个要删除的对象. 以备进行返回.
            UserInfo toDelete = *it;
            applyList->erase(it);
            return toDelete;
        }
    }
    return UserInfo();
}

void DataCenter::createGroupChatSessionAsync(const QList<QString> &userIdList)
{
    netClient.createGroupChatSession(loginSessionId, userIdList);
}

void DataCenter::getMemberListAsync(const QString &chatSessionId)
{
    netClient.getMemberList(loginSessionId, chatSessionId);
}

void DataCenter::resetMemberList(const QString &chatSessionId, const QList<bite_im::UserInfo> &List)
{
    QList<UserInfo>& currentMemberList = (*this->memberList)[chatSessionId];
    currentMemberList.clear();
    for(const auto& m : List)
    {
        UserInfo userInfo;
        userInfo.load(m);
        currentMemberList.push_back(userInfo);
    }
}

QList<UserInfo> *DataCenter::getMemberList(const QString &chatSessionId)
{
    // 没有该会话的成员数据时返回 nullptr, 调用方据此判定"还没加载".
    // 注意条件不能写反: 写反之后 "有数据" 反而返回 nullptr, 群成员永远渲染不出来.
    // 这里也不能用 operator[] 去隐式插入, 否则 "查过" 和 "加载过" 就无法区分了.
    if (!this->memberList->contains(chatSessionId)) {
        return nullptr;
    }
    return &(*this->memberList)[chatSessionId];
}

void DataCenter::searchUserAsync(const QString &searchKey)
{
    netClient.searchUser(loginSessionId, searchKey);
}

void DataCenter::resetSearchUserResult(const QList<bite_im::UserInfo> &userList)
{
    if (searchUserResult == nullptr) {
        searchUserResult = new QList<UserInfo>();
    }
    searchUserResult->clear();

    for (const auto& u : userList) {
        UserInfo userInfo;
        userInfo.load(u);
        searchUserResult->push_back(userInfo);
    }
}

QList<UserInfo> *DataCenter::getSearchUserResult()
{
    return searchUserResult;
}

void DataCenter::searchMessageAsync(const QString &searchKey)
{
    netClient.searchMessage(loginSessionId, this->currentChatSessionId, searchKey);
}

void DataCenter::resetSearchMessageResult(const QList<bite_im::MessageInfo> &msgList)
{
    if(this->searchMessageResult == nullptr)
    {
        this->searchMessageResult = new QList<Message>();
    }
    this->searchMessageResult->clear();
    for(const auto& m : msgList)
    {
        Message message;
        message.load(m);
        searchMessageResult->push_back(message);
    }
}

QList<Message> *DataCenter::getSearchMessageResult()
{
    return searchMessageResult;
}

void DataCenter::searchMessageByTimeAsync(const QDateTime &begTime, const QDateTime &endTime)
{
    netClient.searchMessageByTime(loginSessionId, currentChatSessionId, begTime, endTime);
}

void DataCenter::userLoginAsync(const QString &username, const QString &password)
{
    // 登录操作, 没有 loginSessionId.
    // 登录成功之后, 服务器才会返回 loginSessionId
    netClient.userLogin(username, password);
}

void DataCenter::resetLoginSessionId(const QString &loginSessionId)
{
    this->loginSessionId = loginSessionId;

    // 一旦会话 id 改变, 就需要保存到硬盘上.
    saveDataFile();
}

void DataCenter::userRegisterAsync(const QString &username, const QString &password)
{
    netClient.userRegister(username, password);
}

void DataCenter::phoneLoginAsync(const QString &phone, const QString &verifyCode)
{
    netClient.phoneLogin(phone, this->currentVerifyCodeId, verifyCode);
}

void DataCenter::phoneRegisterAsync(const QString &phone, const QString &verifyCode)
{
    netClient.phoneRegister(phone, this->currentVerifyCodeId, verifyCode);
}

void DataCenter::speechConvertTextAsync(const QString &fileId, const QByteArray &cont)
{
    netClient.speechConvertText(loginSessionId, fileId, cont);
}




void DataCenter::resetMyself(std::shared_ptr<bite_im::GetUserInfoRsp> resp)
{
    // 判断是否已经操作数据
    if(myself == nullptr)
    {
        myself = new UserInfo();
    }
    // 把数据进行接收
    const bite_im::UserInfo& userInfo = resp->userInfo();
    myself->load(userInfo);
}

void DataCenter::getFriendListAsync()
{
    netClient.getFriendList(loginSessionId);
}
}

