#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include "verifycodewidget.h"

// 登录窗口：输入用户名、密码和验证码，完成登录或注册
class LoginWidget : public QWidget
{
    Q_OBJECT
public:
    // 构造函数：创建输入框、按钮等界面元素
    explicit LoginWidget(QWidget *parent = nullptr);

    // 在"登录"和"注册"两种模式之间切换
    void switchMode();

    // 点击提交按钮：根据当前模式发起登录或注册请求
    void clickSubmitBtn();
    // 登录响应返回后的处理
    void userLoginDone(bool ok, const QString& reason);
    // 注册响应返回后的处理
    void userRegisterDone(bool ok, const QString& reason);
protected:
    // 重写：绘制登录窗口背景图
    void paintEvent(QPaintEvent* event) override;
private:
    // 当前是否为登录模式：true 为登录，false 为注册
    bool isLoginMode = true;

    // 用户名输入框
    QLineEdit* usernameEdit;
    // 密码输入框
    QLineEdit* passwordEdit;
    // 验证码输入框（注册时使用）
    QLineEdit* verifyCodeEdit;
    // 图形验证码控件
    VerifyCodeWidget* verifyCodeWidget;

    // 标题：显示"登录"或"注册"
    QLabel* titleLabel;
    // 提交按钮
    QPushButton* submitBtn;
    // 切换到手机号登录
    QPushButton* phoneModeBtn;
    // 切换登录/注册模式
    QPushButton* switchModeBtn;
signals:
};

#endif // LOGINWIDGET_H

