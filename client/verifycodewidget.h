// 本文件：图形验证码控件。随机生成验证码并绘制出来，鼠标点击可刷新。

#ifndef VERIFYCODEWIDGET_H
#define VERIFYCODEWIDGET_H

#include <QWidget>
#include <QRandomGenerator>

// 图形验证码控件：负责生成、绘制和校验验证码
class VerifyCodeWidget : public QWidget
{
    Q_OBJECT
public:
    // 构造函数：初始化随机数生成器并生成第一个验证码
    explicit VerifyCodeWidget(QWidget *parent = nullptr);

    // 通过这个函数, 生成随机的验证码字符串
    QString generateVerifyCode();

    // 重新生成验证码并显示到界面上
    void refreshVerifyCode();

    // 检验验证码是否匹配
    bool checkVerifyCode(const QString& verifyCode);

    // 绘制验证码字符到控件上
    void paintEvent(QPaintEvent* event) override;

    // 用户点击的时候, 刷新验证码, 并重新显示.
    void mousePressEvent(QMouseEvent* event) override;

private:
    // 随机数生成器
    QRandomGenerator randomGenerator;

    // 保存验证码的值
    QString verifyCode = "";

signals:
};

#endif // VERIFYCODEWIDGET_H
