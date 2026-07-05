#include "ui/mainwindow/mainwindow.h"
#include "ui/theme/app_style.hpp"
#ifdef LOG_PANEL
    #include "ui/log_panel/log_panel.h"
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

    #ifdef LOG_PANEL
    qRegisterMetaType<QtMsgType>("QtMsgType");
    auto logPanel = new Kites::LogPanel();
    qInstallMessageHandler(Kites::LogPanel::logHandler);
    logPanel->show();
    
    qDebug() << "This is a debug message";
    qWarning() << "This is a warning message";
    qCritical() << "This is a critical message";
    qInfo() << "This is an info message";
    #endif

    a.setStyle(new Kites::AppStyle(QStyleFactory::create("Fusion")));
    
    Kites::MainWindow w;
    w.setWindowState(Qt::WindowMaximized);
    w.show();

    
    int returnValue = a.exec();
    qInstallMessageHandler(nullptr);
    return returnValue;
}
