#include "src/mainwindow.h"

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

    a.setStyle(QStyleFactory::create("Fusion"));
    Kites::MainWindow w;
    w.show();
    return a.exec();
}
