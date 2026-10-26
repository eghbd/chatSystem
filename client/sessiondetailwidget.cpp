#include "sessiondetailwidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QFontMetrics>
#include <QMessageBox>
#include "debug.h"
#include "choosefrienddialog.h"
#include "model/datacenter.h"
//SessionDetailWidget::SessionDetailWidget(QWidget* parent, const UserInfo& userInfo) : QDialog(parent), userInfo(userInfo)
SessionDetailWidget::SessionDetailWidget(QWidget* parent, const UserInfo& userInfo) : QDialog(parent), userInfo(userInfo) {
    // 1. 设置基本属性
    this->setWindowTitle("会话详情");
    this->setWindowIcon(QIcon(":/resource/image/logo.png"));
    this->setFixedSize(300, 300);
    this->setStyleSheet("QWidget { background-color: rgb(255, 255, 255); }");
    this->setAttribute(Qt::WA_DeleteOnClose);

    // 2. 创建布局管理器
    QGridLayout* layout = new QGridLayout();
    layout->setSpacing(10);
    layout->setContentsMargins(50, 0, 50, 0);
    this->setLayout(layout);

    // 3. 添加 "创建群聊" 按钮
    AvatarItem* createGroupBtn = new AvatarItem(QIcon(":/resource/image/cross.png"), "添加");
    layout->addWidget(createGroupBtn, 0, 0);

    // 4. 添加当前用户的信息 (临时构造的假数据)
    AvatarItem* currentUser;
#if TEST_UI
    currentUser = new AvatarItem(QIcon(":/resource/image/defaultAvatar.png"), "张三123456");
    layout->addWidget(currentUser, 0, 1);
#endif
    currentUser = new AvatarItem(userInfo.avatar, userInfo.userName);
    layout->addWidget(currentUser, 0, 1);

    // 5. 添加 "删除好友" 按钮
    deleteFriendBtn = new QPushButton();
    deleteFriendBtn->setFixedHeight(50);
    deleteFriendBtn->setText("删除好友");
    QString style = "QPushButton { border: 1px solid rgb(90, 90, 90); border-radius: 5px; } ";
    style += "QPushButton:pressed { background-color: rgb(235, 235, 235); }";
    deleteFriendBtn->setStyleSheet(style);
    layout->addWidget(deleteFriendBtn, 1, 0, 1, 3);

    // 6. 添加信号槽, 处理点击 "创建群聊" 按钮
    connect(createGroupBtn->getAvatar(), &QPushButton::clicked, this, [=]() {
        ChooseFriendDialog* chooseFriendDialog = new ChooseFriendDialog(this, userInfo.userId);
        chooseFriendDialog->exec();
    });

    connect(deleteFriendBtn, &QPushButton::clicked, this, &SessionDetailWidget::clickDeleteFriendBtn);
}




/////////////////////////////////////////////
/// 点击 "删除好友" 按钮的槽函数
/////////////////////////////////////////////
void SessionDetailWidget::clickDeleteFriendBtn()
{
    // 1. 二次确认，防止误触
    QMessageBox::StandardButton ret = QMessageBox::question(
        this,
        "删除好友",
        QString("确定要删除好友 \"%1\" 吗？").arg(userInfo.userName),
        QMessageBox::Yes | QMessageBox::No
    );
    if (ret != QMessageBox::Yes) {
        return; // 用户点了"否"：什么都不做
    }

    // 2. TODO: 调用服务端删除好友接口（网络层尚未接入）
    //    例如：FriendService::deleteFriend(userInfo.userId);
    //    成功后刷新好友列表，并发出信号通知主界面
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    dataCenter->deleteFriendAsync(this->userInfo.userId);
    // 3. 当前阶段：删除确认后直接关闭详情窗口
    this->close();
}


/////////////////////////////////////////////
/// 表示一个头像 +  一个名字组合控件
/////////////////////////////////////////////

AvatarItem::AvatarItem(const QIcon &avatar, const QString &name)
{
    // 1. 设置自身的基本属性
    this->setFixedSize(70, 80);

    // 2. 创建布局管理器
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignHCenter);
    this->setLayout(layout);

    // 3. 创建头像
    avatarBtn = new QPushButton();
    avatarBtn->setFixedSize(45, 45);
    avatarBtn->setIconSize(QSize(45, 45));
    avatarBtn->setIcon(avatar);
    avatarBtn->setStyleSheet("QPushButton { border: none; }");

    // 4. 创建名字
    nameLabel = new QLabel();
    nameLabel->setText(name);
    QFont font("微软雅黑", 12);
    nameLabel->setFont(font);
    nameLabel->setAlignment(Qt::AlignCenter);

    // 5. 对名字做 "截断操作"
    const int MAX_WIDTH = 65;
    QFontMetrics metrics(font);
    int totalWidth = metrics.horizontalAdvance(name);
    if (totalWidth >= MAX_WIDTH) {
        // 需要截断
        QString tail = "...";
        int tailWidth = metrics.horizontalAdvance(tail);
        int availableWidth = MAX_WIDTH - tailWidth;
        int availableSize = name.size() * ((double)availableWidth / totalWidth);
        QString newName = name.left(availableSize);
        nameLabel->setText(newName + tail);
    }

    // 6.
    layout->addWidget(avatarBtn);
    layout->addWidget(nameLabel);
}