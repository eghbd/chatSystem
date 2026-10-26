#include "messageeditarea.h"
#include <QVBoxLayout>
#include <QScrollBar>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include "historymessagewidget.h"
#include "model/datacenter.h"
#include "toast.h"
#include "mainwidget.h"

#include "soundrecorder.h"
MessageEditArea::MessageEditArea(QWidget *parent)
    : QWidget{parent}
{
// ============ 输入面板构造函数（聊天窗口底部） ============
// 整体结构：外层垂直布局 vlayout，内部再套一个水平布局 hlayout 放按钮
// ┌──────────────────────────────────────┐
// │  [图片] [文件] [语音] [历史]           │ ← hlayout（顶部按钮行）
// ├──────────────────────────────────────┤
// │                                      │
// │          多行输入框 textEdit           │ ← vlayout 直接添加
// │                                      │
// └──────────────────────────────────────┘

// 1. 设置面板自身属性：固定高度 200px，水平方向随窗口拉伸，垂直方向固定
    this->setFixedHeight(200);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // 2. 创建外层垂直布局（管理整块面板的纵向排布）
    //    按钮行在上、输入框在下；间距 0，下边距留 10px 给输入框贴底
    QVBoxLayout* vlayout = new QVBoxLayout();
    vlayout->setSpacing(0);
    vlayout->setContentsMargins(10, 0, 10, 10);
    this->setLayout(vlayout);

    // 3. 创建水平布局（放置顶部一排功能按钮）
    //    左对齐、顶部对齐，按钮靠左上角排列
    QHBoxLayout* hlayout = new QHBoxLayout();
    hlayout->setSpacing(0);
    hlayout->setContentsMargins(10, 0, 0, 0);
    hlayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    vlayout->addLayout(hlayout);           // 按钮行挂到垂直布局的最上方

    // 4. 创建上方四个功能按钮，统一尺寸并添加到水平布局中
    //    按钮样式：浅灰底、无边框，按下时变白（简单的按压反馈）
    QString btnStyle = "QPushButton { background-color: rgb(245, 245, 245); border: none; } QPushButton:pressed { background-color: rgb(255, 255, 255); }";
    QSize btnSize(35, 35);                 // 按钮固定尺寸 35×35
    QSize iconSize(25, 25);                // 图标尺寸 25×25（居中显示）

    sendImageBtn = new QPushButton();         // 发送图片按钮
    sendImageBtn->setFixedSize(btnSize);
    sendImageBtn->setIconSize(iconSize);
    sendImageBtn->setIcon(QIcon(":/resource/image/image.png"));
    sendImageBtn->setStyleSheet(btnStyle);
    hlayout->addWidget(sendImageBtn);

    sendFileBtn = new QPushButton();          // 发送文件按钮
    sendFileBtn->setFixedSize(btnSize);
    sendFileBtn->setIconSize(iconSize);
    sendFileBtn->setIcon(QIcon(":/resource/image/file.png"));
    sendFileBtn->setStyleSheet(btnStyle);
    hlayout->addWidget(sendFileBtn);

    sendSpeechBtn = new QPushButton();        // 语音输入按钮
    sendSpeechBtn->setFixedSize(btnSize);
    sendSpeechBtn->setIconSize(iconSize);
    sendSpeechBtn->setIcon(QIcon(":/resource/image/sound.png"));
    sendSpeechBtn->setStyleSheet(btnStyle);
    hlayout->addWidget(sendSpeechBtn);

    historyBtn = new QPushButton();           // 聊天记录按钮
    historyBtn->setFixedSize(btnSize);
    historyBtn->setIconSize(iconSize);
    historyBtn->setIcon(QIcon(":/resource/image/history.png"));
    historyBtn->setStyleSheet(btnStyle);
    hlayout->addWidget(historyBtn);

    // 5. 添加多行编辑框（用户输入消息的主体区域）
    //    水平和垂直都随窗口拉伸，透明背景、无边框，字号 14px，内边距 10px
    textEdit = new QPlainTextEdit();
    textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    textEdit->setStyleSheet("QPlainTextEdit { border: none; background-color: transparent; font-size: 14px; padding: 10px; }");
    textEdit->verticalScrollBar()->setStyleSheet("QScrollBar:vertical { width: 2px; background-color: rgb(45, 45, 45); }");  // 滚动条做窄做深，适配深色主题
    vlayout->addWidget(textEdit);          // 输入框挂到按钮行下方，占据剩余空间


    // 6. 添加提示 "录制中" 这样的 QLabel
    tipLabel = new QLabel();
    tipLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tipLabel->setText("录音中...");
    tipLabel->setAlignment(Qt::AlignCenter);
    tipLabel->setFont(QFont("微软雅黑", 24, 600));
    vlayout->addWidget(tipLabel);
    tipLabel->hide();

    // // 7. 添加发送文本消息的按钮
    send = new QPushButton();
    send->setText("发送");
    send->setFixedSize(120, 40);
    QString style = "QPushButton { font-size: 16px; color: rgb(7, 193, 96); border: none; background-color: rgb(233, 233, 233); border-radius: 10px; } ";
    style += "QPushButton:hover { background-color: rgb(210, 210, 210); }";
    style += "QPushButton:pressed { background-color: rgb(190, 190, 190); }";
    send->setStyleSheet(style);
    vlayout->addWidget(send, 0, Qt::AlignRight | Qt::AlignVCenter);

    // // 8. 统一初始化信号槽
    initSignalSlot();
}

void MessageEditArea::initSignalSlot()
{
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    connect(historyBtn, &QPushButton::clicked, this, [=]()
    {
        if(dataCenter->getCurrentChatSessionId().isEmpty())
        {
            return;
        }
        HistoryMessageWidget* historyMessageWidget = new HistoryMessageWidget(this);
        historyMessageWidget->exec();
    });
    // 关联发送文本信息
    connect(send, &QPushButton::clicked, this, &MessageEditArea::sendTextMessage);

    connect(dataCenter, &model::DataCenter::sendMessageDone,
            this, &MessageEditArea::addSelfMessage);
    connect(dataCenter, &model::DataCenter::receiveMessageDone, this, &MessageEditArea::addOtherMessage);
    connect(sendImageBtn, &QPushButton::clicked, this, &MessageEditArea::clickSendImageBtn);
    connect(sendFileBtn, &QPushButton::clicked, this, &MessageEditArea::clickSendFileBtn);
    connect(sendSpeechBtn, &QPushButton::pressed, this, &MessageEditArea::soundRecordPressed);
    connect(sendSpeechBtn, &QPushButton::released, this, &MessageEditArea::soundRecordReleased);
    SoundRecorder* soundRecorder = SoundRecorder::getInstance();
    connect(soundRecorder, &SoundRecorder::soundRecordDone, this, &MessageEditArea::sendSpeech);
}

void MessageEditArea::sendTextMessage()
{
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    if(dataCenter->getCurrentChatSessionId().isEmpty()){
        LOG << "当前未选中任何会话";
        // 这个是弹窗
        Toast::showMessage("当前未选中会话，不发送任何信息");
        return;
    }
    // 拿到输入框内容
    const QString& cont = textEdit->toPlainText().trimmed();
    if(cont.isEmpty())
    {
        LOG << "输入框为空";
        Toast::showMessage("输入框为空");
        return;
    }
    // 输入后清空输入框
    textEdit->setPlainText("");
    LOG << "[发送消息] 提交文本, chatSessionId=" << dataCenter->getCurrentChatSessionId()
        << ", content=" << cont;
    // 通过网络发送数据
    dataCenter->sendTextMessageAsync(dataCenter->getCurrentChatSessionId(), cont);
}

void MessageEditArea::addSelfMessage(const QString &chatSessionId,
                                     MessageType messageType,
                                     const QByteArray &content,
                                     const QString &extraInfo)
{
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    UserInfo* myself = dataCenter->getMyself();
    if (myself == nullptr) {
        LOG << "send succeeded, but current user data is not ready";
        return;
    }
    // 构造一个消息对象
    Message message = Message::makeMessage(
                messageType, chatSessionId, *myself, content, extraInfo);
    dataCenter->addMessage(message);
    LOG << "[发送消息] HTTP 成功，执行本地回显, chatSessionId=" << chatSessionId;
    // 把消息显示在消息展示区
    if (dataCenter->getCurrentChatSessionId() == chatSessionId) {
        MainWidget* mainWidget = MainWidget::getInstance();
    MessageShowArea* messageShow = mainWidget->getMessageShowArea();
        messageShow->addMessage(false, message);
    // 控制消息展示区的滚动条
        messageShow->scrollToEnd();
    }
    // 发送消息通知会话列表
    // 谁负责更新界面，谁就接收这个信号
    emit dataCenter->updataLastMessage(chatSessionId);
}

void MessageEditArea::addOtherMessage(const Message &message)
{
    LOG << "[接收消息] 显示服务端推送, chatSessionId=" << message.sessionId
        << ", sender=" << message.sender.userId << ", type=" << message.messageType;
    MainWidget* mainWidget = MainWidget::getInstance();
    MessageShowArea* messageShowArea = mainWidget->getMessageShowArea();

    messageShowArea->addMessage(true, message);

    messageShowArea->scrollToEnd();

    Toast::showMessage("收到新消息");
}

void MessageEditArea::clickSendImageBtn()
{
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    const QString chatSessionId = dataCenter->getCurrentChatSessionId();
    if (chatSessionId.isEmpty())
    {
        Toast::showMessage("当前没选择会话");
        return;
    }
    QString filter = "Image Files (*.png *.jpg *.jpeg)";
    QString imagePath = QFileDialog::getOpenFileName(this, "选择图片", QDir::homePath(), filter);
    if (imagePath.isEmpty()) {
        return;
    }

    const QByteArray imageContent = Utils::loadFileToByteArray(imagePath);
    QImage image;
    if (imageContent.isEmpty() || !image.loadFromData(imageContent)) {
        Toast::showMessage("图片读取失败");
        return;
    }

    dataCenter->sendImageMessageAsync(chatSessionId, imageContent);



}

void MessageEditArea::soundRecordPressed()
{
    // 判定当前是否选中会话.
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    if (dataCenter->getCurrentChatSessionId().isEmpty()) {
        LOG << "未选中任何会话, 不能发送语音消息";
        Toast::showMessage("当前没选择会话");
        return;
    }

    // 切换语音按钮的图标
    sendSpeechBtn->setIcon(QIcon(":/resource/image/sound_active.png"));

    // 开始录音
    SoundRecorder* soundRecorder = SoundRecorder::getInstance();
    if (!soundRecorder->startRecord()) {
        sendSpeechBtn->setIcon(QIcon(":/resource/image/sound.png"));
        return;
    }

    tipLabel->show();
    textEdit->hide();
}

void MessageEditArea::soundRecordReleased()
{
    // 切换语音按钮的图标
    sendSpeechBtn->setIcon(QIcon(":/resource/image/sound.png"));

    // 停止录音
    SoundRecorder* soundRecorder = SoundRecorder::getInstance();
    soundRecorder->stopRecord();

    tipLabel->hide();
    textEdit->show();
}

void MessageEditArea::sendSpeech(const QString &path)
{
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    const QString chatSessionId = dataCenter->getCurrentChatSessionId();
    if (chatSessionId.isEmpty()) {
        Toast::showMessage("当前没选择会话");
        return;
    }
    // 1. 读取到语音文件的内容
    QByteArray content = Utils::loadFileToByteArray(path);
    if (content.isEmpty()) {
        LOG << "语音文件加载失败";
        Toast::showMessage("录音数据为空");
        return;
    }
    dataCenter->sendSpeechMessageAsync(chatSessionId, content);
}

void MessageEditArea::clickSendFileBtn()
{
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    const QString chatSessionId = dataCenter->getCurrentChatSessionId();
    if (chatSessionId.isEmpty())
    {
        Toast::showMessage("当前没选择会话");
        return;
    }
    QString filter = "*";
    QString path = QFileDialog::getOpenFileName(this, "选择文件", QDir::homePath(), filter);
    if (path.isEmpty()) {
        return;
    }

    const QByteArray content = Utils::loadFileToByteArray(path);
    if (content.isEmpty()) {
        Toast::showMessage("文件读取失败");
        return;
    }
    // 读取文件内容
    // 可能出现文件较大的情况，这种情况需要编写专门的网络接口，实现分页传输
    // 这里直接简单粗暴点
    QFileInfo fileInfo(path);
    const QString& fileName = fileInfo.fileName();

    dataCenter->sendFileMessageAsync(chatSessionId, fileName, content);
}
