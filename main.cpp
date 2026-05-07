#include "src/mainwindow.h"
//#define DEBUG
#ifdef DEBUG
    #include "ui/debug/log_panel.h"
#endif

#include <QApplication>
#include <QStyleFactory> // For setting the style
#include <QPalette>      // For setting the colors
#include <QColor>
#include <QMetaType>
#include <QList>
#include <QString>



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Register Qt metatype for queued signal/slot delivery of QList<QString>
    qRegisterMetaType<QList<QString>>("QList<QString>");

    #ifdef DEBUG
    qRegisterMetaType<QtMsgType>("QtMsgType");
    auto logPanel = new Kites::LogPanel();
    qInstallMessageHandler(Kites::LogPanel::logHandler);
    logPanel->show();
    
    qDebug() << "This is a debug message";
    qWarning() << "This is a warning message";
    qCritical() << "This is a critical message";
    qInfo() << "This is an info message";
    #endif

    a.setStyle(QStyleFactory::create("Fusion"));
    Kites::MainWindow w;
    w.setWindowState(Qt::WindowMaximized);
    w.show();

    
    int returnValue = a.exec();
    qInstallMessageHandler(nullptr);
    return returnValue;
}
