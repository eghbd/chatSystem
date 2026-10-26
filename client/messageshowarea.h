#ifndef MESSAGESHOWAREA_H
#define MESSAGESHOWAREA_H

#include <QScrollArea>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include "model/data.h"
class MessageShowArea : public QScrollArea
{
    Q_OBJECT
public:
    MessageShowArea();
    void addMessage(bool isLeft, const Message& message);
    void addFrontMessage(bool isLeft, const Message& message);
    void clean();
    void scrollToEnd();
private:
    QWidget* container;
};
////////////////////
/// 消息类
/// 消息可能是文本，图片，文件，语音等
class MessageItem : public QWidget{
    Q_OBJECT
public:
    MessageItem(bool isLeft);
    // 工厂方法
    static MessageItem* makeMessageItem(bool isLeft, const Message& message);
    // 添加工厂函数
    static QWidget* makeTextMessageItem(bool isLeft, const QString& text);
    static QWidget* makeImageMessageItem(bool isLeft, const QString& fileId, const QByteArray& content);
    static QWidget* makeFileMessageItem(bool isLeft, const Message& message);
    static QWidget* makeSpeechMessageItem(bool isLeft, const Message& message);

private:
    bool isLeft;
};

class MessageContentLabel : public QWidget{
public:
    // fileName 可以作为 text 的一部分, 传递进来. 不需要单独列一个参数
    // 构造函数：根据消息类型创建文本/文件/语音消息气泡
    MessageContentLabel(const QString& text, bool isLeft, MessageType messageType, const QString& fileId,
                        const QByteArray& content);

    // 绘制消息气泡的背景
    void paintEvent(QPaintEvent* event) override;
    // 鼠标点击：文件消息打开文件、语音消息播放语音
    void mousePressEvent(QMouseEvent* event) override;

    // 文件/图片内容下载完成后刷新气泡显示
    void updateUi(const QString& fileId, const QByteArray& fileContent);
    // 把文件内容另存到本地
    void saveAsFile(const QByteArray& content);

    // 语音播放结束后的回调
    void playDone();

    // 右键菜单：语音消息提供"转文字"等操作
    void contextMenuEvent(QContextMenuEvent* event) override;
    // 语音转文字结果返回后的处理
    void speechConvertTextDone(const QString& fileId, const QString& text);

private:
    QLabel* label;
    bool isLeft; //判断是否是左侧消息

    MessageType messageType;
    QString fileId;
    QByteArray cont;
    QString fileName;
    bool loadContentDone = false;
};
//////////////////////////////////
/// 图片消息
///
class MessageImageLabel : public QWidget{
    Q_OBJECT
public:

    MessageImageLabel(const QString& fileId, const QByteArray& cont, bool isLeft);
    // 更新图片消息
    void updateUI(const QString& fileId, const QByteArray& cont);
    // 绘制图片
    void paintEvent(QPaintEvent* event);
private:
    QPushButton* imageBtn;
    QString fileId;
    QByteArray cont;
    bool isLeft; // 用于判断是左右哪边发的消息



};

#endif // MESSAGESHOWAREA_H
