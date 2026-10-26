#include "mainwidget.h"
#include "ui_mainwidget.h"
#include <QFile>
#include <QHBoxLayout>
#include <QTimer>
#include "model/data.h"
#include "sessionfriendarea.h"
#include "debug.h"
#include "messageeditarea.h"
#include "selfinfowidget.h"
#include "sessiondetailwidget.h"
#include "groupsessiondetailwidget.h"
#include "addfrienddialog.h"
#include "model/datacenter.h"
#include "toast.h"
// 单例模式
MainWidget* MainWidget::instance = nullptr;
MainWidget* MainWidget::getInstance()
{
    if(instance == nullptr)
    {
        instance = new MainWidget();
    }
    return instance;
}


MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWidget)
{
    ui->setupUi(this);
    this -> setWindowTitle("聊天软件");
    this -> setWindowIcon(QIcon(":/resource/image/logo.png"));
    // QString resPath = ":/resource/image/logo.png";
    // QFile f(resPath);
    // qDebug() << "资源是否存在：" << f.exists();
    // QIcon icon(resPath);
    // qDebug() << "icon.isNull() = " << icon.isNull();
    // this->setWindowIcon(icon);
    initMainWindow();
    initLeftWindow();
    initMidWindow();
    initRightWindow();
    initSignalSlot(); // 放在最后：确保所有控件（含 extraBtn 等）都已创建完成再 connect，否则悬空指针崩溃
    model::DataCenter::getInstance()->initWebsocket();
}

MainWidget::~MainWidget()
{
    delete ui;
}
// 初始化整个窗口的大小
void MainWidget::initMainWindow()
{
    QHBoxLayout* layout = new QHBoxLayout();
    // 间隔
    layout->setSpacing(0);
    // 元素距离边界距离
    layout->setContentsMargins(0, 0, 0, 0);
    this -> setLayout(layout);

    windowLeft = new QWidget();
    windowMid = new QWidget();
    windowRight = new QWidget();
    // 窗口宽度
    windowLeft->setFixedWidth(70);
    windowMid->setFixedWidth(200);
    windowRight->setFixedWidth(600);

    // 背景颜色
    // 左栏：导航区，QQ 灰色通用背景
    windowLeft->setStyleSheet("QWidget { background-color: rgb(245, 246, 250); }");   // #F5F6FA
    // 中栏：会话/好友列表，白色
    windowMid->setStyleSheet("QWidget { background-color: rgb(255, 255, 255); }");    // #FFFFFF
    // 右栏：聊天区，纯白
    windowRight->setStyleSheet("QWidget { background-color: rgb(255, 255, 255); }");  // #FFFFFF

    layout->addWidget(windowLeft);
    LOG << "左窗口创建成功";
    layout->addWidget(windowMid);
    LOG << "中窗口创建成功";
    layout->addWidget(windowRight);
    LOG << "右窗口创建成功";

};

void MainWidget::initLeftWindow()
{
    // 创建垂直布局：左栏里的所有按钮从上往下排列
    QVBoxLayout* layout = new QVBoxLayout();

    // 按钮与按钮之间的垂直间距 = 20 像素
    layout->setSpacing(20);

    // 内容与布局四边的距离（左、上、右、下）
    // 上边距
    layout->setContentsMargins(0, 30, 0, 0);

    // 把这个布局挂到左栏窗口上，之后 addWidget 进来的控件都归它管理
    windowLeft->setLayout(layout);


    // 用户头像按钮
    userAvatar = new QPushButton();
    userAvatar->setFixedSize(45, 45);
    userAvatar->setIconSize(QSize(45, 45));
    userAvatar->setIcon(QIcon(":/resource/image/defaultAvatar.png"));
    userAvatar->setStyleSheet("QPushButton { background-color: transparent; border: none;}");
    layout->addWidget(userAvatar, 1, Qt::AlignTop | Qt::AlignHCenter);

    // 添加会话标签按钮
    sessionTabBtn = new QPushButton();
    sessionTabBtn->setFixedSize(45, 45);
    sessionTabBtn->setIconSize(QSize(30, 30));
    sessionTabBtn->setIcon(QIcon(":/resource/image/session_active.png"));
    sessionTabBtn->setStyleSheet("QPushButton { background-color: transparent; border: none;}");
    layout->addWidget(sessionTabBtn, 1, Qt::AlignTop | Qt::AlignHCenter);

    // 添加好友标签按钮
    friendTabBtn = new QPushButton();
    friendTabBtn->setFixedSize(45, 45);
    friendTabBtn->setIconSize(QSize(30, 30));
    friendTabBtn->setIcon(QIcon(":/resource/image/friend_inactive.png"));
    friendTabBtn->setStyleSheet("QPushButton { background-color: transparent; border: none;}");
    layout->addWidget(friendTabBtn, 1, Qt::AlignTop | Qt::AlignHCenter);

    // 添加好友申请标签页按钮
    applyTabBtn = new QPushButton();
    applyTabBtn->setFixedSize(45, 45);
    applyTabBtn->setIconSize(QSize(30, 30));
    applyTabBtn->setIcon(QIcon(":/resource/image/apply_inactive.png"));
    applyTabBtn->setStyleSheet("QPushButton { background-color: transparent; border: none;}");
    layout->addWidget(applyTabBtn, 1, Qt::AlignTop | Qt::AlignHCenter);

    layout->addStretch(20);
}

void MainWidget::initMidWindow()
{
    LOG << "开始创建中窗口内容";
    QGridLayout* layout = new QGridLayout();
    // 距离上方有 20px 的距离, 另外三个方向都不要边距
    layout->setContentsMargins(0, 20, 0, 0);
    layout->setHorizontalSpacing(0);
    layout->setVerticalSpacing(10);
    windowMid->setLayout(layout);

    searchEdit = new QLineEdit();
    searchEdit->setFixedHeight(30);
    searchEdit->setPlaceholderText("搜索");
    searchEdit->setStyleSheet("QLineEdit { border-radius: 5px; background-color: rgb(226, 226, 226); padding-left: 5px;}");

    addFriendBtn = new QPushButton();
    addFriendBtn->setFixedSize(30, 30);
    addFriendBtn->setIcon(QIcon(":/resource/image/cross.png"));
    QString style = "QPushButton { border-radius: 5px; background-color: rgb(226, 226, 226); }";
    style += " QPushButton:pressed { background-color: rgb(240, 240, 240); }";
    addFriendBtn->setStyleSheet(style);

    sessionFriendArea = new SessionFriendArea(); // 赋给成员（原来用局部变量遮蔽了同名成员，成员指针会一直悬空）

    // 为了更灵活的控制边距，只影响搜索框按钮组这一行，不影响下方列表这一行
    // 创建空白的 widget 填充到布局管理器中.
    QWidget* spacer1 = new QWidget();
    spacer1->setFixedWidth(10);
    QWidget* spacer2 = new QWidget();
    spacer2->setFixedWidth(10);
    QWidget* spacer3 = new QWidget();
    spacer3->setFixedWidth(10);
    layout->addWidget(spacer1, 0, 0);
    layout->addWidget(searchEdit, 0, 1);
    layout->addWidget(spacer2, 0, 2);
    layout->addWidget(addFriendBtn, 0, 3);
    layout->addWidget(spacer3, 0, 4);
    layout->addWidget(sessionFriendArea, 1, 0, 1, 5);

}
void MainWidget::initRightWindow()
{
    // 1. 创建右侧窗口的布局管理器
    QVBoxLayout* vlayout = new QVBoxLayout();
    vlayout->setSpacing(0);
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setAlignment(Qt::AlignTop);
    windowRight->setLayout(vlayout);

    // 2. 创建上方标题栏
    QWidget* titleWidget = new QWidget();
    titleWidget->setFixedHeight(62);
    titleWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    titleWidget->setObjectName("titleWidget");
    titleWidget->setStyleSheet("#titleWidget { border-bottom: 1px solid rgb(230, 230, 230); border-left: 1px solid rgb(230, 230, 230); }");
    vlayout->addWidget(titleWidget);

    // 3. 给标题栏, 添加标题 label 和 一个按钮
    QHBoxLayout* hlayout = new QHBoxLayout();
    hlayout->setSpacing(0);
    // 使标题的 label 和 按钮距离左右两侧的边界, 有点间距.
    hlayout->setContentsMargins(10, 0, 10, 0);
    titleWidget->setLayout(hlayout);
    sessionTitleLabel = new QLabel();
    sessionTitleLabel->setStyleSheet("QLabel { font-size: 22px; border-bottom: 1px solid rgb(230, 230, 230);}");
#if TEST_UI
    // 为了测试界面临时增加的. 实际这里的内容, 应该是使用从服务器获取的数据来设置.
    sessionTitleLabel->setText("这是会话标题");
#endif
    hlayout->addWidget(sessionTitleLabel);
    extraBtn = new QPushButton();
    extraBtn->setFixedSize(30, 30);
    extraBtn->setIconSize(QSize(30, 30));
    extraBtn->setIcon(QIcon(":/resource/image/more.png"));
    extraBtn->setStyleSheet("QPushButton { border:none; background-color: rgb(245, 245, 245); } QPushButton:pressed { background-color: rgb(220, 220, 220); }");
    hlayout->addWidget(extraBtn);

    messageShowArea = new MessageShowArea;
    vlayout->addWidget(messageShowArea);

    messageEditArea = new MessageEditArea();
    vlayout->addWidget(messageEditArea, 0, Qt::AlignBottom);
}
// 用于链接的信号槽
void MainWidget::initSignalSlot()
{
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    connect(dataCenter, &model::DataCenter::getChatSessionListDone, this, [this]() {
        chatSessionListLoading = false;
        if (activeTab == SESSION_LIST) {
            updateChatSessionList();
        }
        if (!pendingSwitchUserId.isEmpty()) {
            const QString userId = pendingSwitchUserId;
            pendingSwitchUserId.clear();
            switchSession(userId);
        }
    });
    connect(sessionTabBtn, &QPushButton::clicked, this, &MainWidget::switchTabToSession);
    connect(friendTabBtn, &QPushButton::clicked, this, &MainWidget::switchTabToFriend);
    connect(applyTabBtn, &QPushButton::clicked, this, &MainWidget::switchTabToApply);
    connect(userAvatar, &QPushButton::clicked, this, [=](){
        SelfInfoWidget* selfInfoWidget = new SelfInfoWidget(this);
        selfInfoWidget->exec(); // 弹出对话框
    });
    /////////////////////////////////////////////
    /// 点击会话详情按钮, 弹出会话详情窗口
    /////////////////////////////////////////////
    connect(extraBtn, &QPushButton::clicked, this, [=]() {
        #if TEST_GROUP_SESSION_DETAIL
                bool isSingleChat = false;
        #else
                bool isSingleChat = true;
        #endif
        ChatSession* chatSessionInfo = dataCenter->findChatSessionById(dataCenter->getCurrentChatSessionId());
        // 判定当前会话是单聊还是群聊.
        // 这个 bool 变量, 要根据当前选中的实际会话来确定. (后续接入服务器, 获取真实数据之后)
        if (isSingleChat) {
            // 单聊, 弹出这个窗口
            UserInfo* userInfo = dataCenter->findFriendById(chatSessionInfo->userId);
            if (userInfo == nullptr) {
                LOG << "单聊会话对应的用户不存在, 无法弹出会话详情窗口";
                return;
            }
            SessionDetailWidget* sessionDetailWidget = new SessionDetailWidget(this, *userInfo);
            sessionDetailWidget->exec();
        } else {
            // 群聊, 弹出另一个窗口
            GroupSessionDetailWidget* groupSessionDetailWidget = new GroupSessionDetailWidget(this);
            groupSessionDetailWidget->exec();
        }
    });
    connect(addFriendBtn, &QPushButton::clicked, this, [=]()
    {
        AddFriendDialog* addFriendDialog = new AddFriendDialog(this);
        addFriendDialog->exec();
    });
    //////////////////////////////////
    /// 修改搜索框内容
    //////////////////////////////////
    connect(searchEdit, &QLineEdit::textEdited, this, [=]() {
        // 取出当前输入框的内容, 设置到新弹框的输入框里面
        const QString& searchKey = searchEdit->text();
        AddFriendDialog* addFriendDialog = new AddFriendDialog(this);
        addFriendDialog->setSearchKey(searchKey);
        // 清空主窗口的文本内容
        searchEdit->setText("");
        addFriendDialog->exec();
    });
    //////////////////////////////////
    /// 获取个人信息
    //////////////////////////////////
    connect(dataCenter, &model::DataCenter::getMyselfDone, this, [=]()
    {
        auto myself = dataCenter->getMyself();
        userAvatar->setIcon(myself->avatar);
    });
    dataCenter->getMyselfAsync();
    /////////////////////////////
    /// 获取好友列表
    /// ///////////////////////////
    loadFriendList();
    /////////////////////////////
    /// 申请好友列表
    /// ///////////////////////////
    loadApplyList();
    // 会话列表提前加载，保证点击好友时可以直接找到对应会话。
    loadSessionList();
    //////////////////////////
    /// 处理头像
    /// //////////////////
    connect(dataCenter, &model::DataCenter::changeAvatarDone, this, [=](){
        UserInfo* myself = dataCenter->getMyself();
        userAvatar->setIcon(myself->avatar);
    });
    /////////////////////////////////////////////
    /// 处理删除好友
    /////////////////////////////////////////////
    connect(dataCenter, &model::DataCenter::deleteFriendDone, this, [=]() {
        // 更新会话列表和好友列表
        this->updateFriendList();
        this->updateChatSessionList();
        LOG << "删除好友完成";
    });

    connect(dataCenter, &model::DataCenter::clearCurrentSession, this, [=]() {
        sessionTitleLabel->setText("");
        messageShowArea->clean();
        dataCenter->setCurrentChatSessionId("");
        LOG << "清空当前会话完成";
    });

    /////////////////////////////////////////////
    /// 处理添加好友申请
    /////////////////////////////////////////////
    connect(dataCenter, &model::DataCenter::addFriendApplyDone, this, [=]() {
        Toast::showMessage("好友申请已发送");
    });

    /////////////////////////////////////////////
    /// 处理添加好友申请推送数据
    /////////////////////////////////////////////
    connect(dataCenter, &model::DataCenter::receiveFriendApplyDone, this, [=]() {
        this->updateApplyList();
        Toast::showMessage("收到新的好友申请");
    });

    /////////////////////////////////////////////
    /// 处理同意好友申请
    /////////////////////////////////////////////
    connect(dataCenter, &model::DataCenter::acceptFriendApplyDone, this, [=]() {
        this->updateApplyList();
        this->updateFriendList();
        Toast::showMessage("好友已经添加完成");
    });

    /////////////////////////////////////////////
    /// 处理拒绝好友申请
    /////////////////////////////////////////////
    connect(dataCenter, &model::DataCenter::rejectFriendApplyDone, this, [=]() {
        // 需要更新好友申请列表. 刚才拒绝的这一项, 是需要删除掉的.
        this->updateApplyList();
        Toast::showMessage("好友申请已经拒绝");
    });

    /////////////////////////////////////////////
    /// 处理好友申请结果的推送数据
    /////////////////////////////////////////////
    connect(dataCenter, &model::DataCenter::receiveFriendProcessDone, this, [=](const QString& nickname, bool agree) {
        if (agree) {
            // 同意
            this->updateFriendList();
            Toast::showMessage(nickname + " 已经同意了你的好友申请");
        } else {
            // 拒绝
            Toast::showMessage(nickname + " 已经拒绝了你的好友申请");
        }
    });

    /////////////////////////////////////////////
    /// 处理创建群聊的响应信号
    /////////////////////////////////////////////
    connect(dataCenter, &model::DataCenter::createGroupChatSessionDone, this, [=]() {
        Toast::showMessage("创建群聊会话请求已经发送!");
    });

    /////////////////////////////////////////////
    /// 处理创建会话的推送数据
    /////////////////////////////////////////////
    connect(dataCenter, &model::DataCenter::receiveSessionCreateDone, this, [=]() {
        this->updateChatSessionList();
        // 通知用户, 入群
        Toast::showMessage("您被拉入到新的群聊中!");
    });
}

void MainWidget::initWebsocket()
{
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    dataCenter->initWebsocket();
}
void MainWidget::switchTabToSession()
{
    activeTab = SESSION_LIST;
    sessionTabBtn->setIcon(QIcon(":/resource/image/session_active.png"));
    friendTabBtn->setIcon(QIcon(":/resource/image/friend_inactive.png"));
    applyTabBtn->setIcon(QIcon(":/resource/image/apply_inactive.png"));
    this->loadSessionList();
}
void MainWidget::switchTabToFriend()
{
    // 1. 记录当前切换到了哪个标签页
    activeTab = FRIEND_LIST;
    // 2. 调整图标显示情况, 把会话的按钮图标设为 active, 另外两个图标设为 inactive.
    friendTabBtn->setIcon(QIcon(":/resource/image/friend_active.png"));
    sessionTabBtn->setIcon(QIcon(":/resource/image/session_inactive.png"));
    applyTabBtn->setIcon(QIcon(":/resource/image/apply_inactive.png"));
    // 3. 在主窗口的中间部分, 加载出会话列表数据
    this->loadFriendList();
}
void MainWidget::switchTabToApply()
{
    // 1. 记录当前切换到了哪个标签页
    activeTab = APPLY_LIST;
    // 2. 调整图标显示情况, 把会话的按钮图标设为 active, 另外两个图标设为 inactive.
    applyTabBtn->setIcon(QIcon(":/resource/image/apply_active.png"));
    sessionTabBtn->setIcon(QIcon(":/resource/image/session_inactive.png"));
    friendTabBtn->setIcon(QIcon(":/resource/image/friend_inactive.png"));
    // 3. 在主窗口的中间部分, 加载出会话列表数据
    this->loadApplyList();
}

void MainWidget::loadSessionList()
{
    // 先判定会话列表数据是否在本地 (DataCenter) 中存在. 如果本地存在, 直接构造界面内容.
    // 如果本地不存在, 则从服务器获取数据.
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    if (dataCenter->getChatSessionList() != nullptr) {
        // 从内存加载数据显示
        updateChatSessionList();
    } else {
        // 从网络加载数据
        if (!chatSessionListLoading) {
            chatSessionListLoading = true;
            dataCenter->getChatSessionListAsync();
            QTimer::singleShot(5000, this, [this]() {
                chatSessionListLoading = false;
            });
        }
    }
}

void MainWidget::loadFriendList()
{
    // 好友列表在DataCenter中存储
    // 首先对DataCenter进行判断，判断是否有数据
    // 好友列表数据是在 DataCenter 中存储的
    // 首先需要判定 DataCenter 中是否已经有数据了. 如果有数据, 直接加载本地的数据.
    // 如果没有数据, 从服务器获取
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    if (dataCenter->getFriendList() != nullptr) {
        // 从内存这个列表中加载数据
        updateFriendList();
    } else {
        // 通过网络来加载数据
        connect(dataCenter, &model::DataCenter::getFriendListDone, this, &MainWidget::updateFriendList, Qt::UniqueConnection);
        dataCenter->getFriendListAsync();
    }
}

void MainWidget::loadApplyList()
{
    // 好友申请列表在 DataCenter 中存储的
    // 首先判定 DataCenter 本地是否已经有数据了. 如果有, 直接加载到界面上.
    // 如果没有则需要从服务器获取
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    if (dataCenter->getApplyList() != nullptr) {
        // 本地有数据, 直接加载
        updateApplyList();
    } else {
        // 本地没有数据, 通过网络加载
        connect(dataCenter, &model::DataCenter::getApplyListDone, this, &MainWidget::updateApplyList, Qt::UniqueConnection);
        dataCenter->getApplyListAsync();
    }
}

void MainWidget::updateFriendList()
{
    // 如果当前标签页不是好友列表，就不渲染任何数据到界面上
    if(activeTab != FRIEND_LIST)return;
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    QList<UserInfo>* friendList = dataCenter->getFriendList();
    //清空界面上数据
    sessionFriendArea->clear();
    //
    // 遍历好友列表, 添加到界面上
    for (const auto& f : *friendList) {
        sessionFriendArea->addItem(FriendItemType, f.userId, f.avatar, f.userName, f.des);
    }
}

void MainWidget::updateChatSessionList()
{
    if (activeTab != SESSION_LIST) {
        // 当前标签页不是会话列表, 不渲染任何数据到界面上
        return;
    }
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    QList<ChatSession>* chatSessionList = dataCenter->getChatSessionList();
    sessionFriendArea->clear();

    for (const auto& c : *chatSessionList) {
        if (c.newMessage.messageType == TEXT_TYPE) {
            sessionFriendArea->addItem(SessionItemType, c.sessionId, c.avatar, c.sessionName, c.newMessage.cont);
        } else if (c.newMessage.messageType == IMAGE_TYPE) {
            sessionFriendArea->addItem(SessionItemType, c.sessionId, c.avatar, c.sessionName, "[图片]");
        } else if (c.newMessage.messageType == FILE_TYPE) {
            sessionFriendArea->addItem(SessionItemType, c.sessionId, c.avatar, c.sessionName, "[文件]");
        } else if (c.newMessage.messageType == SPEECH_TYPE) {
            sessionFriendArea->addItem(SessionItemType, c.sessionId, c.avatar, c.sessionName, "[语音]");
        } else {
            LOG << "错误的消息类型! messageType=" << c.newMessage.messageType;
        }
    }
}

void MainWidget::updateApplyList()
{
    if (activeTab != APPLY_LIST) {
        // 当前标签页不是 "好友申请列表", 界面上不做任何渲染
        return;
    }
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    QList<UserInfo>* applyList = dataCenter->getApplyList();

    sessionFriendArea->clear();

    for (const auto& u : *applyList) {
        // 直接把这个 UserInfo 对象添加到界面上即可, 不涉及到 PB 和 自己的结构之间的转换.
        // 此处 UserInfo 的 description 不需要填写进来. 好友申请列表中, 不显示用户的签名的 (这个位置被替换成了两个按钮)
        sessionFriendArea->addItem(ApplyItemType, u.userId, u.avatar, u.userName, "");
    }
}

void MainWidget::loadRecentMessage(const QString &chatSessionId)
{
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    if(dataCenter->getRecentMessageList(chatSessionId) != nullptr)
    {
        updateRecentMessage(chatSessionId);
    }
    else
    {
        connect(dataCenter, &model::DataCenter::getRecentMessageListDone, this, &MainWidget::updateRecentMessage, Qt::UniqueConnection);
        // 本地没有数据时
        dataCenter->getRecentMessageListAsync(chatSessionId, true);
    }
}

void MainWidget::updateRecentMessage(const QString &chatSessionId)
{
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    auto* recentMessageList = dataCenter->getRecentMessageList(chatSessionId);

    messageShowArea->clean();
    //根据当前拿到的消息列表, 显示到界面上
    //    此处把数据显示到界面上, 可以使用头插, 也可以使用尾插.
    //    这里打算使用头插的方式来进行实现.
    //    主要因为消息列表来说, 用户首先看到的, 应该是 "最近" 的消息, 也就是 "末尾" 的消息.
    for (int i = recentMessageList->size() - 1; i >= 0; --i) {
        const Message& message = recentMessageList->at(i);
        bool isLeft = message.sender.userId != dataCenter->getMyself()->userId;
        messageShowArea->addFrontMessage(isLeft, message);
    }

    //  设置会话标题
    ChatSession* chatSessionInfo = dataCenter->findChatSessionById(chatSessionId);
    if (chatSessionInfo != nullptr) {
        // 把会话名称显示到界面上.
        sessionTitleLabel->setText(chatSessionInfo->sessionName);
    }

    //  保存当前选中的会话是哪个.
    dataCenter->setCurrentChatSessionId(chatSessionId);

    //  自动把滚动条, 滚动到末尾
    messageShowArea->scrollToEnd();


}

void MainWidget::switchSession(const QString &userId)
{
    // 1. 在会话列表中, 先找到对应的会话元素
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    if (dataCenter->getChatSessionList() == nullptr) {
        pendingSwitchUserId = userId;
        loadSessionList();
        return;
    }
    ChatSession* chatSessionInfo = dataCenter->findChatSessionByUserId(userId);
    if (chatSessionInfo == nullptr) {
        // 正常来说, 每个好友, 都会有一个对应的会话(哪怕从来没说过话).
        // 添加好友的时候, 就创建出来的会话.
        LOG << "[严重错误] 当前选中的好友, 对应的会话不存在!";
        return;
    }

    // 2. 把选中的会话置顶, 把这个会话信息放到整个会话列表的第一个位置.
    //    后续在界面显示的时候, 就是按照列表的顺序, 从前往后显示的.
    dataCenter->topChatSessionInfo(*chatSessionInfo);

    // 3. 切换到会话列表标签页
    switchTabToSession();

    // 4. 加载这个会话对应的历史消息. 刚刚做了一个 "置顶操作" , 被选中的好友对应的会话, 在会话列表的最前头, 也就是 0 号下标.
    sessionFriendArea->clickItem(0);
}

MessageShowArea *MainWidget::getMessageShowArea()
{
    return messageShowArea;
}
