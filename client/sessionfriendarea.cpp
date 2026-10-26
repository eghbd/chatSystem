#include "sessionfriendarea.h"
#include <QScrollBar>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPushButton>
#include <QLabel>
#include <QString>
#include <QPainter>
#include <QStyleOption>
#include <QObject>
#include <QTimer>
#include "model/data.h"
#include "model/datacenter.h"
#include "debug.h"
#include "mainwidget.h"
SessionFriendArea::SessionFriendArea(QWidget *parent)
    : QScrollArea{parent}
{
    // 滚动条
    this->setWidgetResizable(true);
    // LOG << "滚动条设置成功";

    // 滚动条外观
    this->verticalScrollBar()->setStyleSheet("QScrollBar:vertical { width: 2px; background-color: rgb(240, 240, 240); }");
    this->horizontalScrollBar()->setStyleSheet("QScrollBar:horizontal { height: 0;}");
    this->setStyleSheet("QScrollArea { border: none; }");
    //LOG << "滚动条外观设置成功";

    // 创建 Container 这样的 widget, 作为包含内部元素的容器
    container = new QWidget();
    this->setWidget(container);

    // 给 container 添加布局管理器
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    container->setLayout(layout);
    // 图鉴测试
    // QPushButton* button = new QPushButton();
    // button->setText("按钮");
    // layout->addWidget(button);
    //  最后把布局放入 container
    container->setLayout(layout);
// 测试
#if TEST_UI
    QIcon icon(":/resource/image/defaultAvatar.png");
    for (int i = 0; i < 10; ++i) {
        this->addItem(SessionItemType, QString::number(i), icon, "张三" + QString::number(i), "最后一条消息" + QString::number(i));
        // LOG << "添加成功";
    }
#endif
}

void SessionFriendArea::clear()
{
    QLayout* layout = container->layout();
    for(int i=layout->count()-1;i >= 0;i--)
    {
        // 移除下标元素
        QLayoutItem* item = layout->takeAt(i);
        if(item->widget())
        {
            delete item->widget();
        }
    }
}

void SessionFriendArea::addItem(ItemType itemType, const QString& id, const QIcon &avatar, const QString &name, const QString &text)
{
    SessionFriendItem* item = nullptr;
    if (itemType == SessionItemType) {
        item = new SessionItem(this, id, avatar, name, text);
    } else if (itemType == FriendItemType) {
        item = new FriendItem(this, id, avatar, name, text);
    } else if (itemType == ApplyItemType) {
        item = new ApplyItem(this, id, avatar, name);
    } else {
        LOG << "错误的 ItemType! itemType=" << itemType;
        return;
    }
    container->layout()->addWidget(item);
}
// 选择聊天对象
// 有的元素是聊天好友，好友头像，好友姓名，最新消息预览
SessionFriendItem::SessionFriendItem(QWidget *owner, const QIcon &avatar, const QString &name, const QString &text)
    : owner(owner)
{
    this->setFixedHeight(70);
    this->setStyleSheet("QWidget { background-color : rgb(231, 231, 231); }");

    //创建网格管理器
    QGridLayout* layout = new QGridLayout();
    layout->setContentsMargins(10, 0, 0, 0);
    layout->setSpacing(0);
    layout->setHorizontalSpacing(10);
    layout->setVerticalSpacing(0);
    this->setLayout(layout);

    //好友头像
    QPushButton* avatarButton = new QPushButton();
    avatarButton->setFixedSize(50, 50);
    avatarButton->setIconSize(QSize(50, 50));
    avatarButton->setIcon(avatar);
    avatarButton->setStyleSheet("QPushButton {border: none}");
    avatarButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    //好友名字
    QLabel* nameLabel = new QLabel();
    nameLabel->setText(name);
    nameLabel->setStyleSheet("QLabel { font-size: 18px; font-weight: 600; }");
    nameLabel->setFixedHeight(35);
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // 显示最新一条信息
    messageLabel = new QLabel();
    messageLabel->setText(text);
    messageLabel->setFixedHeight(35);
    messageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    //LOG << "好友聊天页创建成功";
    layout->addWidget(avatarButton, 0, 0, 2, 2);
    layout->addWidget(nameLabel, 0, 2, 1, 8);
    layout->addWidget(messageLabel, 1, 2, 1, 8);

}
// 特殊设置，为了让子类也能影响父类背景的设置
void SessionFriendItem::paintEvent(QPaintEvent *event)
{
    (void) event;
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void SessionFriendItem::mousePressEvent(QMouseEvent *event)   // 鼠标按下事件
{
    (void) event;
    selectFriend();
}

void SessionFriendItem::selectFriend()
{
    // 拿到父容器（container）的所有子对象
    const QObjectList children = this->parentWidget()->children();

    // 遍历每个子对象
    for(QObject* child : children)
    {
        if(!child->isWidgetType())      // 不是控件类型（比如布局对象）→ 跳过
        {
            continue;
        }

        // 把子对象转成 SessionFriendItem*（动态类型转换）
        SessionFriendItem* item = dynamic_cast<SessionFriendItem*>(child);
        if(item == nullptr) // 容器里可能混入非列表项控件，跳过，防止空指针崩溃
        {
            continue;
        }

        // 如果这个列表项当前是"选中"状态 → 取消它的选中（背景恢复浅灰）
        if(item->selected)
        {
            item->selected = false;
            item->setStyleSheet("QWidget {background-color: rgb(231, 231, 231);}");
        }

        // 当前点击的项：设为选中 + 高亮背景（210 比 231 深，视觉上"凸起"）
    }

    this->setStyleSheet("QWidget {background-color: rgb(210, 210, 210);}");
    this->selected = true;
    this->active();
}

void SessionFriendItem::active()
{
    // 点击之后, 要激活对应的会话列表元素
    //LOG << "点击 FriendItem 触发的逻辑! userId=" << userId;

    // MainWidget* mainWidget = MainWidget::getInstance();
    // mainWidget->switchSession(userId);
}
// 鼠标进入列表项时触发
void SessionFriendItem::enterEvent(QEnterEvent *event)
{
    (void) event;
    if(!this->selected) // 只有"没被选中"的项才变,防止把选中项目也改变颜色
    {
        this->setStyleSheet("QWidget {background-color: rgb(220, 220, 220);}");
        // 背景 231 → 220
    }
}

// 鼠标离开列表项时触发
void SessionFriendItem::leaveEvent(QEvent *event)
{
    (void) event;
    if(!this->selected)  // 只有"没被选中"的项才变
    {
        this->setStyleSheet("QWidget {background-color: rgb(231, 231, 231);}");
        // 背景 220 → 231（恢复默认）
    }
}

void SessionFriendArea::clickItem(int index)
{
    if (index < 0 || index >= container->layout()->count()) {
        LOG << "点击元素的下标超出范围! index=" << index;
        return;
    }
    QLayoutItem* layoutItem = container->layout()->itemAt(index);
    if (layoutItem == nullptr || layoutItem->widget() == nullptr) {
        LOG << "指定的元素不存在! index=" << index;
        return;
    }
    SessionFriendItem* item = dynamic_cast<SessionFriendItem*>(layoutItem->widget());
    item->selectFriend();
}

////////////////////
/// 会话item实现
SessionItem::SessionItem(QWidget *owner, const QString &chatId, const QIcon &avatar, const QString &name, const QString &lastMessage)
    : chatId(chatId),
    SessionFriendItem(owner, avatar, name, lastMessage),
    text(lastMessage)
{
    // 更新最后一个学习的信号
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    connect(dataCenter, &model::DataCenter::updataLastMessage,
            this, &SessionItem::updataLastMessage);
    // 未读消息展示
    // 未读消息可能离线也在涨，所以我们需要每次初始化时都能保证正确接收,所以放在初始化里进行加载
    int unread = dataCenter->getUnread(chatId);
    if(unread > 0)
    {
        // 存在未读消息
        this->messageLabel->setText("[未读" + QString::number(unread) + "消息]" + text);

    }

}

void SessionItem::updataLastMessage(const QString &chatSessionId)
{
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    // 1. 判定 chatSessionId 是否匹配
    if (this->chatId != chatSessionId) {
        // 当前 SessionItem 不是你正在发消息的 SessionItem!
        return;
    }
    // chatSessionId 匹配, 真正更新最后一条消息!!

    // 2. 把最后一条消息, 获取到.
    QList<Message>* messageList = dataCenter->getRecentMessageList(chatSessionId);
    if (messageList == nullptr || messageList->size() == 0) {
        // 当前会话没有任何消息, 无需更新
        return;
    }

    const Message& lastMessage = messageList->back();

    // 3. 明确显示的文本内容
    //    由于消息有四种类型.
    //    文本消息, 直接显示消息的内容; 图片消息, 直接显示 "[图片]"; 文件消息, 直接显示 "[文件]"; 语音消息, 直接显示 "[语音]"
    if (lastMessage.messageType == TEXT_TYPE) {
        text = lastMessage.cont;
    } else if (lastMessage.messageType == IMAGE_TYPE) {
        text = "[图片]";
    } else if (lastMessage.messageType == FILE_TYPE) {
        text = "[文件]";
    } else if (lastMessage.messageType == SPEECH_TYPE) {
        text = "[语音]";
    } else {
        LOG << "错误的消息类型!";
        return;
    }

    // 4. 把这个内容, 显示到界面上
    //    针对这里的逻辑, 后续还需要考虑到 "未读消息" 情况. 关于未读消息的处理, 后续编写 "接收消息" 的时候再处理.
    //    先判定, 当前消息的会话, 是不是正在选中的会话. 如果是, 不会更新任何未读消息.
    //    如果不是, 看未读消息是否 > 0, 并且做出前缀的拼装
    if (chatSessionId == dataCenter->getCurrentChatSessionId()) {
        this->messageLabel->setText(text);
    } else {
        int unread = dataCenter->getUnread(chatSessionId);
        if (unread > 0) {
            this->messageLabel->setText(QString("[未读%1条] ").arg(unread) + text);
        }
    }

}
void SessionItem::active()
{
    // 点击之后, 要加载会话的历史消息列表
    LOG << "点击 SessionItem 触发的逻辑! chatSessionId=" << chatId;

    // 加载会话历史消息, 即会涉及到当前内存的数据操作, 又会涉及到网络通信, 还涉及到界面的变更.
    MainWidget* mainWidget = MainWidget::getInstance();
    mainWidget->loadRecentMessage(chatId);

    // 清空未读消息的数据, 并且更新显示
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    dataCenter->clearUnread(chatId);

    // 更新界面的显示. 把会话消息预览这里, 前面的 "[未读x条]" 内容给干掉
    this->messageLabel->setText(text);
}
/////////
/// 好友申请item
ApplyItem::ApplyItem(QWidget *owner, const QString &userId, const QIcon &avatar, const QString &name)
    : SessionFriendItem(owner, avatar, name, ""), userId(userId)
{
    // 1. 移除父类的 messageLabel
    QGridLayout* layout = dynamic_cast<QGridLayout*>(this->layout());
    layout->removeWidget(messageLabel);
    // 要记得释放内存, 否则会内存泄露.
    delete messageLabel;

    // 2. 创建两个按钮出来
    QPushButton* acceptBtn = new QPushButton();
    acceptBtn->setText("同意");
    QPushButton* rejectBtn = new QPushButton();
    rejectBtn->setText("拒绝");

    // 3. 添加到布局管理器中
    layout->addWidget(acceptBtn, 1, 2, 1, 1);
    layout->addWidget(rejectBtn, 1, 3, 1, 1);

    // 4. 添加信号槽
    connect(acceptBtn, &QPushButton::clicked, this, &ApplyItem::acceptFriendApply);
    connect(rejectBtn, &QPushButton::clicked, this, &ApplyItem::rejectFriendApply);


}

void ApplyItem::active()
{
    LOG << "申请好友界面加载成功";
}

void ApplyItem::acceptFriendApply()
{
    // 发送网络请求, 告知服务器, 同意了.
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    // 同意谁的好友申请了.
    // 针对这个操作, 信号处理, 是需要更新好友列表以及好友申请列表. 直接在主窗口中处理更合适的.
    dataCenter->acceptFriendApplyAsync(this->userId);
}

void ApplyItem::rejectFriendApply()
{
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    dataCenter->rejectFriendApplyAsync(this->userId);
}
///////
/// 好友item
FriendItem::FriendItem(QWidget *owner, const QString &userId, const QIcon &avatar, const QString &name, const QString &description)
    : SessionFriendItem(owner, avatar, name, description), userId(userId)
{

}

void FriendItem::active()
{
    MainWidget* mainWidget = MainWidget::getInstance();
    const QString targetUserId = userId;
    QTimer::singleShot(0, mainWidget, [mainWidget, targetUserId]() {
        mainWidget->switchSession(targetUserId);
    });
}


