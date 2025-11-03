#include "src/mainwindow.h"

#include <QApplication>
#include <QStyleFactory> // For setting the style
#include <QPalette>      // For setting the colors
#include <QColor>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //a.setWindowIcon(QIcon(":/icons/kites.png"));
    a.setStyle(QStyleFactory::create("Fusion"));

    // 2. Create a new white/light palette
    //QPalette lightPalette;

    // Set all the colors for a light theme
    // lightPalette.setColor(QPalette::Window, Qt::white);
    // lightPalette.setColor(QPalette::WindowText, Qt::black);
    // lightPalette.setColor(QPalette::Base, QColor(245, 245, 245)); // Lighter gray for text fields
    // lightPalette.setColor(QPalette::AlternateBase, Qt::white);
    // lightPalette.setColor(QPalette::ToolTipBase, Qt::white);
    // lightPalette.setColor(QPalette::ToolTipText, Qt::black);
    // lightPalette.setColor(QPalette::Text, Qt::black);
    // lightPalette.setColor(QPalette::Button, Qt::white);
    // lightPalette.setColor(QPalette::ButtonText, Qt::black);
    // lightPalette.setColor(QPalette::BrightText, Qt::red);
    // lightPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    // lightPalette.setColor(QPalette::Highlight, QColor(42, 130, 218)); // Blue highlight
    // lightPalette.setColor(QPalette::HighlightedText, Qt::white);

    // 3. Apply the new palette to your entire application
    //a.setPalette(lightPalette);

    Kites::MainWindow w;
    w.show();
    return a.exec();
}
