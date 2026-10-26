#pragma once
#include <QString>
#include <QIcon>
#include <QUuid>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>

#include "base.qpb.h"
#include "gateway.qpb.h"
#include "user.qpb.h"
#include "friend.qpb.h"
#include "file.qpb.h"
#include "notify.qpb.h"
#include "speech_recognition.qpb.h"
#include "message_storage.qpb.h"
#include "message_transmit.qpb.h"

// 打印日志
static inline QString getFileName(const QString& path)
{
    QFileInfo fileInfo(path);
    return fileInfo.fileName();
}
#define TAG QString("[%1][%2]:").arg(getFileName(__FILE__), QString::number(__LINE__))
#define LOG qDebug().noquote()<<TAG


// 工具类
namespace Utils{
    // 唯一uid
    static inline QString tempId()
    {
        return "Make:" + QUuid::createUuid().toString().sliced(25, 12);
    }
    // 秒级时间戳 转换成 格式化字符串
    static inline QString formatTime(int64_t timestamp)
    {
        QDateTime time = QDateTime::fromSecsSinceEpoch(timestamp);
        // 自定义格式：年-月-日 时:分:秒
        return time.toString("yyyy-MM-dd HH:mm:ss");
    }
    // 获取当前时间的秒级时间戳，发消息时直接调用
    static inline int64_t getTime()
    {
        return QDateTime::currentSecsSinceEpoch();
    }
    // QByteArray 转成 QIcon
    static inline QIcon makeIcon(const QByteArray& byte)
    {
        QPixmap pix;
        pix.loadFromData(byte);
        QIcon icon(pix);
        return icon;
    }
    // 读取指定文件的二进制内容，从而获取到ByteArray
    static inline QByteArray loadFileToByteArray(const QString& path)
    {
        QFile file(path);
        bool Bool = file.open(QFile::ReadOnly);
        if(!Bool)
        {
            qDebug() << "打开文件失败";
        }
        QByteArray content = file.readAll();
        file.close(); // 打开文件记得关闭
        return content;
    }
    // 把ByteArray内容写入指定文件
    static inline void writeByteArrayToFile(const QString& path, const QByteArray& content)
    {
        QFile file(path);
        bool Bool = file.open(QFile::WriteOnly);
        if(!Bool)
        {
            qDebug() << "打开文件失败";
        }
        file.write(content);
        file.flush();
        file.close();
    }
};

// 用户的基础信息类
class UserInfo{
public:
    QString userId = ""; //用户id
    QString userName = ""; //用户名
    QString phone = ""; //用户手机
    QString des = ""; //签名
    QIcon avatar; //头像
    ///////////////////////////////////////////////
    /// 真正qt与服务器链接部分
    /// 从 protobuffer 的 UserInfo 对象, 转成当前代码的 UserInfo 对象
    //////////////////////////////////////////////
    void load(const bite_im::UserInfo& userInfo) {
        this->userId = userInfo.userId();
        this->userName = userInfo.nickname();
        this->phone = userInfo.phone();
        this->des = userInfo.description();
        if (userInfo.avatar().isEmpty()) {
            // 使用默认头像即可
            this->avatar = QIcon(":/resource/image/defaultAvatar.png");
        } else {
            this->avatar = Utils::makeIcon(userInfo.avatar());
        }
    }
};

// 信息类型
enum MessageType{
    TEXT_TYPE, //文字类型
    FILE_TYPE, //文件类型
    SPEECH_TYPE, //语音类型
    IMAGE_TYPE //图片类型
};


// 信息类
class Message{
public:
    QString messageId = ""; //信息id
    QString sessionId = ""; //会话编号
    // 必须给默认值: ChatSession 里有个 Message newMessage, 会话没有 prevMessage 时
    // 它就是默认构造的, 而 updateChatSessionList 会直接读这个字段 -> 读到栈垃圾是 UB.
    MessageType messageType = TEXT_TYPE; //信息类型
    UserInfo sender; //发送方信息
    QString messageTime = ""; //会话时间
    QByteArray cont; //消息
    QString fileId = ""; //用于设置文件类型
    QString fileName = ""; //文件名称

    // 聊天系统需要面对一对多的局面，存在大量用户信息，可以用到工厂模式
    static Message makeMessage(MessageType messagetype, const QString& sessionId, const UserInfo& sender,
                                const QByteArray& content, const QString& extraInfo)
    {
        //根据传回类型不同，判断不同的函数调用
        if(messagetype == TEXT_TYPE)
        {
            return makeText(sessionId, sender, content);
        }
        else if(messagetype == FILE_TYPE)
        {
            return makeFile(sessionId, sender, content, extraInfo);
        }
        else if(messagetype == SPEECH_TYPE)
        {
            return makeSpeech(sessionId, sender, content, extraInfo);
        }
        else if(messagetype == IMAGE_TYPE)
        {
            return makeImage(sessionId, sender, content);
        }
        else
        {
            return Message();
        }
    }
    // 把 pb 中的 MessageInfo 转换成客户端使用的 Message 对象
    void load(const bite_im::MessageInfo& messageInfo) {
        this->messageId = messageInfo.messageId();
        this->sessionId = messageInfo.chatSessionId();
        this->messageTime = Utils::formatTime(messageInfo.timestamp());
        this->sender.load(messageInfo.sender());

        // 设置消息类型
        auto type = messageInfo.message().messageType();
        if (type == bite_im::MessageTypeGadget::MessageType::STRING) {
            this->messageType = TEXT_TYPE;
            this->cont = messageInfo.message().stringMessage().content().toUtf8();
        } else if (type == bite_im::MessageTypeGadget::MessageType::IMAGE) {
            this->messageType = IMAGE_TYPE;
            if (messageInfo.message().imageMessage().hasImageContent()) {
                this->cont = messageInfo.message().imageMessage().imageContent();
            }
            if (messageInfo.message().imageMessage().hasFileId()) {
                this->fileId = messageInfo.message().imageMessage().fileId();
            }
        } else if (type == bite_im::MessageTypeGadget::MessageType::FILE) {
            this->messageType = FILE_TYPE;
            if (messageInfo.message().fileMessage().hasFileContents()) {
                this->cont = messageInfo.message().fileMessage().fileContents();
            }
            if (messageInfo.message().fileMessage().hasFileId()) {
                this->fileId = messageInfo.message().fileMessage().fileId();
            }
            this->fileName = messageInfo.message().fileMessage().fileName();
        } else if (type == bite_im::MessageTypeGadget::MessageType::SPEECH) {
            this->messageType = SPEECH_TYPE;
            if (messageInfo.message().speechMessage().hasFileContents()) {
                this->cont = messageInfo.message().speechMessage().fileContents();
            }
            if (messageInfo.message().speechMessage().hasFileId()) {
                this->fileId = messageInfo.message().speechMessage().fileId();
            }
        } else {
            // 错误的类型, 啥都不做了, 只是打印一个日志
            LOG << "非法的消息类型! type=" << type;
        }
    }
private:
    static Message makeText(const QString& sessionId,const UserInfo& sender, const QByteArray& content)
    {
        Message message;
        message.messageId = Utils::tempId();
        message.sessionId = sessionId;
        message.messageType = TEXT_TYPE;
        message.sender = sender;
        message.messageTime = Utils::formatTime(Utils::getTime());
        message.cont = content;
        message.fileId = "";
        message.fileName = "";
        return message;
    }
    static Message makeImage(const QString& sessionId,const UserInfo& sender, const QByteArray& content)
    {
        Message message;
        message.messageId = Utils::tempId();
        message.sessionId = sessionId;
        message.messageType = IMAGE_TYPE;
        message.sender = sender;
        message.messageTime = Utils::formatTime(Utils::getTime());
        message.cont = content;
        message.fileId = "";
        message.fileName = "";
        return message;
    }
    static Message makeFile(const QString& sessionId,const UserInfo& sender, const QByteArray& content, const QString& fileName)
    {
        Message message;
        message.messageId = Utils::tempId();
        message.sessionId = sessionId;
        message.messageType = FILE_TYPE;
        message.sender = sender;
        message.messageTime = Utils::formatTime(Utils::getTime());
        message.cont = content;
        message.fileId = "";
        message.fileName = fileName;
        return message;
    }
    static Message makeSpeech(const QString& sessionId,const UserInfo& sender, const QByteArray& content, const QString& extraInfo)
    {
        Message message;
        message.messageId = Utils::tempId();
        message.sessionId = sessionId;
        message.messageType = SPEECH_TYPE;
        message.sender = sender;
        message.messageTime = Utils::formatTime(Utils::getTime());
        message.cont = content;
        message.fileId = "";
        message.fileName = "";
        return message;
    }
};


// 会话类
class ChatSession{
public:
    QString sessionId; //会话id
    QString sessionName; //会话名字
    Message newMessage; //最新一条信息，类似聊天软件会在选择界面只展示最新一条信息
    QString userId;
    QIcon avatar; //会话头像
    void load(const bite_im::ChatSessionInfo& chatSessionInfo) {
        this->sessionId = chatSessionInfo.chatSessionId();
        this->sessionName = chatSessionInfo.chatSessionName();
        if (chatSessionInfo.hasSingleChatFriendId()) {
            this->userId = chatSessionInfo.singleChatFriendId();
        }
        if (chatSessionInfo.hasPrevMessage()) {
            newMessage.load(chatSessionInfo.prevMessage());
        }
        if (chatSessionInfo.hasAvatar() && !chatSessionInfo.avatar().isEmpty()) {
            // 已经有头像了, 直接设置这个头像
            this->avatar = Utils::makeIcon(chatSessionInfo.avatar());
        } else {
            // 如果没有头像, 则根据当前会话是单聊还是群聊, 使用不同的默认头像.
            if (userId != "") {
                // 单聊
                this->avatar = QIcon(":/resource/image/defaultAvatar.png");
            } else {
                // 群聊
                this->avatar = QIcon(":/resource/image/groupAvatar.png");
            }
        }
    }
};
