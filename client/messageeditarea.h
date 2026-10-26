#ifndef MESSAGEEDITAREA_H
#define MESSAGEEDITAREA_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QPlainTextEdit>
#include "model/data.h"
class MessageEditArea : public QWidget
{
    Q_OBJECT
public:
    // 构造函数：创建输入框和各类发送按钮
    explicit MessageEditArea(QWidget *parent = nullptr);

    // 绑定按钮、录音控件等与发送逻辑的信号槽
    void initSignalSlot();
    // 发送文本消息
    void sendTextMessage();
    // 把"自己发送的消息"显示到消息展示区（对方消息用 addOtherMessage）
    void addSelfMessage(const QString& chatSessionId, MessageType messageType,
                        const QByteArray& content, const QString& extraInfo);
    // 把对方发来的消息显示到消息展示区
    void addOtherMessage(const Message& message);

    // 点击发送图片按钮：选择图片并发送
    void clickSendImageBtn();
    // 点击发送文件按钮：选择文件并发送
    void clickSendFileBtn();

    // 按下录音按钮：开始录音
    void soundRecordPressed();
    // 松开录音按钮：结束录音
    void soundRecordReleased();
    // 把录音文件作为语音消息发送
    void sendSpeech(const QString& path);
private:
    QPushButton* sendImageBtn; // 发送照片
    QPushButton* sendFileBtn; // 发送文件
    QPushButton* sendSpeechBtn; // 发送语音
    QPushButton* historyBtn; // 历史聊天按键
    QPlainTextEdit* textEdit; // 输入框
    QPushButton* send; // 发送按钮
    QPushButton *sendFile;
    QLabel* tipLabel;
signals:
};

#endif // MESSAGEEDITAREA_H
