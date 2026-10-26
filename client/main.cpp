#include "mainwidget.h"

#include <QApplication>
#include "model/data.h"
#include "model/datacenter.h"
#include "loginwidget.h"
#include "debug.h"
#include "network/netclient.h"
#include <QFile>
FILE* output = nullptr;
void msgHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QByteArray log = msg.toUtf8();
    fprintf(output, "%s\n", log.constData());
    fflush(output); //确保数据落到硬盘
}
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#if deBug
    output = fopen("./log.txt","a");
    // 不考虑关闭
    qInstallMessageHandler(msgHandler);
#endif
    // LOG << "hello";
#if TEST_SKIP_LOGIN
    MainWidget* w = MainWidget::getInstance();
    w -> show();
#else
    LoginWidget* loginWidget = new LoginWidget();
    loginWidget->show();
#endif
#if TEST_NETWORK
    model::DataCenter* dataCenter = model::DataCenter::getInstance();
    dataCenter->ping();
#endif
    return a.exec();
}
