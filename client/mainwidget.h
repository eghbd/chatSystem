#ifndef MAINWIDGET_H
#define MAINWIDGET_H
#pragma once
#include <QIcon>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>

#include "sessionfriendarea.h"
#include "messageeditarea.h"
#include "messageshowarea.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWidget;
}
QT_END_NAMESPACE

namespace model {
class DataCenter;
}

class MainWidget : public QWidget
{
    Q_OBJECT
// 单例模式
private:
    static MainWidget* instance;
    explicit MainWidget(QWidget *parent = nullptr);
public:
    static MainWidget* getInstance();
    ~MainWidget() override;
private:
    Ui::MainWidget *ui;
    //model::DataCenter* dataCenter = nullptr;

    // 窗口最左侧部分
    QWidget* windowLeft;
    // 窗口中间部分
    QWidget* windowMid;
    // 窗口右侧部分
    QWidget* windowRight;

    // 用户头像
    QPushButton* userAvatar;
    // 会话标签页按钮
    QPushButton* sessionTabBtn;
    // 好友标签页按钮
    QPushButton* friendTabBtn;
    // 好友申请标签页按钮
    QPushButton* applyTabBtn;

    // 搜索框
    QLineEdit* searchEdit;
    // 添加好友按钮
    QPushButton* addFriendBtn;
    //
    SessionFriendArea* sessionFriendArea;
    // 消息编辑
    MessageEditArea* messageEditArea;
    // 消息展示
    MessageShowArea* messageShowArea;
    // 会话详情按钮
    QPushButton* extraBtn;
    // 会话按钮
    QLabel* sessionTitleLabel;
    QString pendingSwitchUserId;
    bool chatSessionListLoading = false;

    enum ActiveTab {
        SESSION_LIST,
        FRIEND_LIST,
        APPLY_LIST
    };

    ActiveTab activeTab = SESSION_LIST;
public:
    void initMainWindow();
    void initLeftWindow();
    void initMidWindow();
    void initRightWindow();

    void initSignalSlot();
    void initWebsocket();

    void switchTabToSession();
    void switchTabToFriend();
    void switchTabToApply();

    void loadSessionList();
    void loadFriendList();
    void loadApplyList();

    void updateFriendList();
    void updateChatSessionList();
    void updateApplyList();

    void loadRecentMessage(const QString& chatSessionId);
    void updateRecentMessage(const QString& chatSessionId);

    // 点击好友项之后, 切换到会话列表的总的函数. 上方的 switchTabToSession 只是其中的一个环节.
    void switchSession(const QString& userId);

    // MessageShowArea* getMessageShowArea();

    MessageShowArea* getMessageShowArea();

};
#endif // MAINWIDGET_H
