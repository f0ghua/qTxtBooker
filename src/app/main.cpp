#include "mainwindow.h"
#include "QAppLogging.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	
    QAppLogging::installHandler();
    //QAppLogging::instance()->setOutputDest(QAppLogging::eDestFile|QAppLogging::eDestSystem);
    //QAppLogging::instance()->setLogFilePath("log_sapaUSBTool.txt");
    qInfo() << "App is initializing ...";
	
    MainWindow w;
    w.show();

    return a.exec();
}
