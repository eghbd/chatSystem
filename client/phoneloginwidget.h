#ifndef PHONELOGINWIDGET_H
#define PHONELOGINWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

// 手机号登录窗口：发送验证码、倒计时、提交登录或注册
class PhoneLoginWidget : public QWidget
{
    Q_OBJECT
public:
    // 构造函数：创建手机号、验证码输入框和按钮
    explicit PhoneLoginWidget(QWidget *parent = nullptr);

    // 点击发送验证码：向当前手机号发送短信验证码
    void sendVerifyCode();
    // 验证码发送成功后的处理：开始倒计时
    void sendVerifyCodeDone();

    // 点击提交按钮：根据当前模式发起手机号登录或注册
    void clickSubmitBtn();
    // 手机号登录响应返回后的处理
    void phoneLoginDone(bool ok, const QString& reason);
    // 手机号注册响应返回后的处理
    void phoneRegisterDone(bool ok, const QString& reason);

    // 验证码倒计时：每秒刷新一次按钮文字
    void countDown();
    // 在"登录"和"注册"两种模式之间切换
    void switchMode();
private:
    // 手机号输入框
    QLineEdit* phoneEdit;
    // 发送验证码按钮
    QPushButton* sendVerifyCodeBtn;
    // 验证码输入框
    QLineEdit* verifyCodeEdit;
    // 标题：显示"登录"或"注册"
    QLabel* titleLabel;
    // 提交按钮
    QPushButton* submitBtn;
    // 切换登录/注册模式
    QPushButton* switchModeBtn;

    // 当前是否为登录模式
    bool isLoginMode = true;
    QString currentPhone = "";    // 记录是使用哪个手机号发送的验证码
    // 验证码倒计时定时器
    QTimer* timer;
    // 倒计时剩余秒数
    int leftTime = 30;

signals:
};

#endif // PHONELOGINWIDGET_H

