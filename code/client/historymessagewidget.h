#ifndef HISTORYMESSAGEWIDGET_H
#define HISTORYMESSAGEWIDGET_H

#include <QDialog>
#include <QWidget>
#include <QLineEdit>
#include <QRadioButton>
#include <QGridLayout>
#include <QDateTimeEdit>
#include "model/data.h"
#include <QLabel>
#include <QPushButton>
////////////////////////////////////////////////////////////////////
/// 表示一个历史消息元素
////////////////////////////////////////////////////////////////////

class HistoryItem : public QWidget {
    Q_OBJECT
public:
    HistoryItem() {}

    // 根据一条消息创建对应的历史消息条目
    static HistoryItem* makeHistoryItem(const Message& message);
};


class HistoryMessageWidget : public QDialog
{
    Q_OBJECT
public:
    // 构造函数：创建搜索条件栏与消息展示区域
    HistoryMessageWidget(QWidget* parent);

    // 在窗口中添加一个历史消息
    void addHistoryMessage(const Message& message);

    // 清空窗口中所有的历史消息
    void clear();

    // 点击搜索按钮：按关键词或时间区间发起历史消息搜索
    void clickSearchBtn();
    // 搜索响应返回后：把结果填充到窗口
    void clickSearchBtnDone();

private:
    // 持有所有的历史消息结果的容器对象
    QWidget* container;

    // 搜索关键字输入框
    QLineEdit* searchEdit;
    // 按关键字搜索的单选按钮
    QRadioButton* keyRadioBtn;
    // 按时间区间搜索的单选按钮
    QRadioButton* timeRadioBtn;
    // 搜索的起始时间
    QDateTimeEdit* begTimeEdit;
    // 搜索的结束时间
    QDateTimeEdit* endTimeEdit;

    // 初始化滚动区域，用于容纳大量历史消息
    void initScrollArea(QGridLayout* layout);
};


////////////////////////////////////////////////////////////////////
/// 展示图片历史消息
////////////////////////////////////////////////////////////////////

class ImageButton : public QPushButton {
public:
    // 构造函数：保存图片对应的文件 id 与图片内容
    ImageButton(const QString& fileId, const QByteArray& content);
    // 图片内容下载完成后刷新显示
    void updateUI(const QString& fileId, const QByteArray& content);

private:
    QString fileId;
};



////////////////////////////////////////////////////////////////////
/// 展示文件历史消息
////////////////////////////////////////////////////////////////////

class FileLabel : public QLabel {
public:
    // 构造函数：保存文件 id 与文件名
    FileLabel(const QString& fileId, const QString& fileName);

    // 文件内容下载完成后保存下来，供点击打开
    void getContentDone(const QString& fileId, const QByteArray& fileContent);

    // 通过这个函数, 来处理鼠标点击操作.
    void mousePressEvent(QMouseEvent* event) override;

private:
    QString fileId;
    QByteArray content;
    QString fileName;
    bool loadDone = false;
};

////////////////////////////////////////////////////////////////////
/// 展示语音历史消息
////////////////////////////////////////////////////////////////////

class SpeechLabel : public QLabel {
public:
    // 构造函数：保存语音对应的文件 id
    SpeechLabel(const QString& fileId);

    // 语音内容下载完成后保存下来，供点击播放
    void getContentDone(const QString& fileId, const QByteArray& content);

    // 通过这个函数处理鼠标点击
    void mousePressEvent(QMouseEvent* event) override;

private:
    QString fileId;
    QByteArray content;
    bool loadDone = false;
};

#endif // HISTORYMESSAGEWIDGET_H
