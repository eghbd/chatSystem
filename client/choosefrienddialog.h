#ifndef CHOOSEFRIENDDIALOG_H
#define CHOOSEFRIENDDIALOG_H

#include <QDialog>
#include <QWidget>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>
class ChooseFriendDialog;

class ChooseFriendItem : public QWidget {
    Q_OBJECT
public:
    // 构造函数：创建头像、昵称和复选框，并记录所属的弹窗对象
    ChooseFriendItem(ChooseFriendDialog* owner, const QString& userId, const QIcon& avatar, const QString& name, bool checked);

    // 重绘条目：根据鼠标是否悬浮绘制不同的背景色
    void paintEvent(QPaintEvent* event) override;
    // 鼠标移入：标记为悬浮状态并重绘
    void enterEvent(QEnterEvent* event) override;
    // 鼠标移出：取消悬浮状态并重绘
    void leaveEvent(QEvent* event) override;

    // 获取该条目对应的用户 id
    const QString& getUserId() const {
        return userId;
    }

    // 获取复选框对象，供外部读写勾选状态
    QCheckBox* getCheckBox() {
        return checkBox;
    }

private:
    bool isHover = false;

    QCheckBox* checkBox;
    QPushButton* avatarBtn;
    QLabel* nameLabel;
    ChooseFriendDialog* owner;			// 记录了哪个 QWidget 持有了这个 Item. 此处的 QWidget 应该是一个 ChooseFriendDialog

    QString userId;   					// 记录了当前 Item 对应的 userId 是啥.
};

class ChooseFriendDialog : public QDialog
{
    Q_OBJECT
public:
    // 构造函数：创建左侧"全部好友"和右侧"已选好友"两块区域
    ChooseFriendDialog(QWidget* parent);
    ChooseFriendDialog(QWidget* parent, const QString& userId);

    // 初始化左侧区域：展示全部好友
    void initLeft(QHBoxLayout* layout);
    // 初始化右侧区域：展示已经勾选的好友
    void initRight(QHBoxLayout* layout);
    // 点击确定：根据已选好友创建群聊会话
    void clickOkBtn();
    // 汇总右侧已选好友，生成群聊成员 id 列表
    QList<QString> generateMemberList();

    // 加载好友数据，把好友逐个添加到左侧列表
    void initData();

    // 往左侧列表添加一个好友条目
    void addFriend(const QString& userId, const QIcon& avatar, const QString& name, bool checked);
    // 往右侧列表添加一个已选好友条目
    void addSelectedFriend(const QString& userId, const QIcon& avatar, const QString& name);
    // 从右侧列表移除一个已选好友条目
    void deleteSelectedFriend(const QString& userId);


private:
    // 保存左侧全部好友列表的 QWidget
    QWidget* totalContainer;
    // 保存右侧选中好友列表的 QWidget
    QWidget* selectedContainer;

    // 当前选择窗口是点击哪个用户来弹出的.
    QString userId;
};

#endif // CHOOSEFRIENDDIALOG_H
