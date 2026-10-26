#ifndef ADDFRIENDDIALOG_H
#define ADDFRIENDDIALOG_H

#include <QDialog>
#include <QGridLayout>
#include <QLineEdit>

#include "model/data.h"

//////////////////////////////////////
/// 表示一个好友搜索的结果
//////////////////////////////////////
class FriendResultItem : public QWidget {
    Q_OBJECT
public:
    // 构造函数：根据用户信息创建头像、昵称和"添加"按钮
    FriendResultItem(const UserInfo& userInfo);
    // 点击"添加"按钮：向该用户发送好友申请
    void clickAddBtn();

private:
    const UserInfo& userInfo;

    // 结果条目中的"添加"按钮
    QPushButton* addBtn;

};

//////////////////////////////////////
/// 整个搜索好友的窗口
//////////////////////////////////////
class AddFriendDialog : public QDialog
{
    Q_OBJECT
public:
    // 构造函数：创建搜索框和搜索结果区域
    AddFriendDialog(QWidget* parent);

    // 初始化结果显示区
    void initResultArea();

    // 往窗口中新增一个好友搜索结果
    void addResult(const UserInfo& userInfo);

    // 清空界面上所有的好友结果
    void clear();

    // 把搜索关键字填入搜索框
    void setSearchKey(const QString& searchKey);

    // 点击搜索按钮：根据关键字发起用户搜索请求
    void clickSearchBtn();
    // 搜索响应返回后：把匹配到的用户填充到结果区域
    void clickSearchBtnDone();

private:
    // 搜索关键字输入框
    QLineEdit* searchEdit;

    // 整个窗口总的网格布局
    QGridLayout* layout;

    // 保存搜索好友的结果
    QWidget* resultContainer;
};


#endif // ADDFRIENDDIALOG_H
