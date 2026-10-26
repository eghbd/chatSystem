#ifndef USERINFOWIDGET_H
#define USERINFOWIDGET_H

#include <QDialog>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include "model/data.h"
////////////////
/// 好友的信息显示窗口
class UserInfoWidget : public QDialog
{
    Q_OBJECT
public:
    UserInfoWidget(const UserInfo& userInfo, QWidget* parent = nullptr);
    void initSignalSlot();
    void clickDeleteFriendBtn();
    void clickApplyBtn();
    void updateButtonState();
private:

    const UserInfo userInfo;

    QPushButton* avatar; // 头像
    QLabel* idTag; // 用户id Tag
    QLabel* idLabel; // 真实用户id
    QLabel* nameTag;
    QLabel* nameLabel;
    QLabel* phoneTag;
    QLabel* phoneLabel;

    QPushButton* apply;
    QPushButton* sendMessage;
    QPushButton* deleteFriend;
};

#endif // USERINFOWIDGET_H
