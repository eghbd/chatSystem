#include "userinfowidget.h"
#include <QGridLayout>
#include "model/datacenter.h"
#include "mainwidget.h"
#include <QMessageBox>
UserInfoWidget::UserInfoWidget(const UserInfo& userInfo, QWidget *parent)
    : QDialog(parent), userInfo(userInfo)
{
    // 1. 设置基本属性
    this->setFixedSize(400, 200);
    this->setWindowTitle("用户详情");
    this->setWindowIcon(QIcon(":/resource/image/logo.png"));
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->move(QCursor::pos());

    // 2. 创建布局管理器
    QGridLayout* layout = new QGridLayout();
    layout->setVerticalSpacing(10);
    layout->setHorizontalSpacing(20);
    layout->setContentsMargins(40, 20, 0, 0);
    layout->setAlignment(Qt::AlignTop);
    this->setLayout(layout);

    // 3. 添加头像
    avatar = new QPushButton();
    avatar->setFixedSize(75, 75);
    avatar->setIconSize(QSize(75, 75));
    avatar->setIcon(userInfo.avatar);

    QString labelStyle = "QLabel { font-weight: 800; padding-left: 20px;}";
    QString btnStyle = "QPushButton { border: 1px solid rgb(100, 100, 100); border-radius: 5px; background-color: rgb(240, 240, 240); }";
    btnStyle += "QPushButton:pressed { background-color: rgb(205, 205, 205); }";
    btnStyle += "QPushButton:disabled { color: rgb(150, 150, 150); border-color: rgb(190, 190, 190); background-color: rgb(225, 225, 225); }";

    int width = 80;
    int height = 30;

    // 4. 添加用户序号
    idTag = new QLabel();
    idTag->setText("序号");
    idTag->setStyleSheet(labelStyle);
    idTag->setFixedSize(width, height);
    idTag->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    idLabel = new QLabel();
    idLabel->setText(userInfo.userId);
    idLabel->setFixedSize(width, height);
    idLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // 5. 添加用户昵称
    nameTag = new QLabel();
    nameTag->setText("昵称");
    nameTag->setStyleSheet(labelStyle);
    nameTag->setFixedSize(width, height);
    nameTag->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    nameLabel = new QLabel();
    nameLabel->setText(userInfo.userName);
    nameLabel->setFixedSize(width, height);
    nameLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // 6. 设置电话
    phoneTag = new QLabel();
    phoneTag->setText("电话");
    phoneTag->setStyleSheet(labelStyle);
    phoneTag->setFixedSize(width, height);
    phoneTag->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    phoneLabel = new QLabel();
    phoneLabel->setText(userInfo.phone);
    phoneLabel->setFixedSize(width, height);
    phoneLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // 7. 添加功能按钮
    apply = new QPushButton();
    apply->setText("申请好友");
    apply->setFixedSize(80, 30);
    apply->setStyleSheet(btnStyle);
    apply->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    sendMessage = new QPushButton();
    sendMessage->setText("发送消息");
    sendMessage->setFixedSize(80, 30);
    sendMessage->setStyleSheet(btnStyle);
    sendMessage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    deleteFriend = new QPushButton();
    deleteFriend->setText("删除好友");
    deleteFriend->setFixedSize(80, 30);
    deleteFriend->setStyleSheet(btnStyle);
    deleteFriend->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // 8. 添加上述内容到布局管理器中
    layout->addWidget(avatar, 0, 0, 3, 1);

    layout->addWidget(idTag, 0, 1);
    layout->addWidget(idLabel, 0, 2);

    layout->addWidget(nameTag, 1, 1);
    layout->addWidget(nameLabel, 1, 2);

    layout->addWidget(phoneTag, 2, 1);
    layout->addWidget(phoneLabel, 2, 2);

    layout->addWidget(apply, 3, 0);
    layout->addWidget(sendMessage, 3, 1);
    layout->addWidget(deleteFriend, 3, 2);

    // 9. 初始化按钮的禁用关系
    //    判定依据就是拿着当前用户的 userId, 在 DataCenter 的好友列表中, 查询即可.
    updateButtonState();

    // 10. 初始化信号槽
    initSignalSlot();
}

void UserInfoWidget::updateButtonState()
{
    model::DataCenter* dataCenter = model::DataCenter::getInstance();

    // 好友列表还没返回时不能猜测关系，先把三个操作都锁住；等列表到达后再刷新。
    if (dataCenter->getFriendList() == nullptr) {
        apply->setEnabled(false);
        sendMessage->setEnabled(false);
        deleteFriend->setEnabled(false);
        connect(dataCenter, &model::DataCenter::getFriendListDone,
                this, &UserInfoWidget::updateButtonState, Qt::UniqueConnection);
        return;
    }

    const bool isFriend = dataCenter->findFriendById(userInfo.userId) != nullptr;
    apply->setEnabled(!isFriend);
    sendMessage->setEnabled(isFriend);
    deleteFriend->setEnabled(isFriend);
}

void UserInfoWidget::initSignalSlot()
{
    connect(sendMessage, &QPushButton::clicked, this, [=](){
        MainWidget* mainWidget = MainWidget::getInstance();
        mainWidget->switchSession(userInfo.userId);
        this->close();
    });
    connect(deleteFriend, &QPushButton::clicked, this, &UserInfoWidget::clickDeleteFriendBtn);
    connect(apply, &QPushButton::clicked, this, &UserInfoWidget::clickApplyBtn);
}

void UserInfoWidget::clickDeleteFriendBtn()
{
    // 1. 弹出对话框, 让用户确认是否要真的删除
    auto result = QMessageBox::warning(this, "确认删除", "确认删除当前好友?", QMessageBox::Ok | QMessageBox::Cancel);
    if (result != QMessageBox::Ok) {
        LOG << "删除好友取消";
        return;
    }
    // 2. 发送网络请求, 实现删除好友功能.
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    // 由于此处信号的处理, 都是由主窗口这里进行的.
    dataCenter->deleteFriendAsync(userInfo.userId);
    // 3. 关闭本窗口
    this->close();
}

void UserInfoWidget::clickApplyBtn()
{
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    dataCenter->addFriendApplyAsync(userInfo.userId);
    this->close();
}
