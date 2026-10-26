#ifndef SESSIONFRIENDAREA_H
#define SESSIONFRIENDAREA_H

#include <QWidget>
#include <QScrollArea>
#include <QPaintEvent>
#include <QLabel>
//#include "model/data.h"
//using model::Message;


//////////////////////////////////////////////////////////
/// 滚动区域中的 Item 的类型
//////////////////////////////////////////////////////////

enum ItemType {
    SessionItemType,
    FriendItemType,
    ApplyItemType
};
////////////////
/// 选择聊天好友界面
class SessionFriendArea : public QScrollArea
{
    Q_OBJECT
public:
    explicit SessionFriendArea(QWidget *parent = nullptr);

    // 清空区域内item
    void clear();
    void clickItem(int index);
    // 添加
    void addItem(ItemType itemType, const QString& id, const QIcon& avatar,
                const QString& name, const QString& text);
private:
    QWidget* container = nullptr;

signals:
};
///////////
/// 好友会话选择 item
///////////
class SessionFriendItem : public QWidget{
    Q_OBJECT
public:
    SessionFriendItem(QWidget* owner, const QIcon& avatar, const QString& name, const QString& text);
    // 特殊函数，官方设置的，用于子类向父类同步背景色
    void paintEvent(QPaintEvent* event) override;
    // 当鼠标点击后实现的事件
    void mousePressEvent(QMouseEvent* event) override;
    void selectFriend();
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

    void select();
    virtual void active();
private:
    QWidget* owner;
    bool selected = false;
    QWidget* container;
protected:
    QLabel* messageLabel;
};

/////////////////////
/// 会话Item
/////////////////
class SessionItem : public SessionFriendItem{
    Q_OBJECT
public:
    SessionItem(QWidget* owner, const QString& chatId, const QIcon& avatar,
                const QString& name, const QString& lastMessage);
    void active() override;
    void updataLastMessage(const QString& chatSessionId);
private:
    QString chatId;
    // 最后一条消息的预览
    QString text;
};
///////////
/// 好友item
class FriendItem : public SessionFriendItem{
    Q_OBJECT
public:
    FriendItem(QWidget* owner, const QString& userId, const QIcon& avatar,
                 const QString& name, const QString& description);
    void active() override;
private:
    QString userId;
};
/////////////
/// 申请好友
class ApplyItem : public SessionFriendItem{
    Q_OBJECT
public:
    ApplyItem(QWidget* owner, const QString& userId, const QIcon& avatar,
              const QString& name);
    void active() override;
    void acceptFriendApply();
    void rejectFriendApply();
private:
    QString userId;
};



#endif // SESSIONFRIENDAREA_H
