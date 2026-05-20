#include "ui/debug/log_panel.h"
#include <QDateTime>
namespace Kites
{
LogPanel *LogPanel::instance = nullptr;
LogPanel::LogPanel(QWidget *parent) : QPlainTextEdit(parent)
{
    instance = this;
    setReadOnly(true);
}

LogPanel::~LogPanel()
{
    if (instance == this)
        instance = nullptr;
}

void LogPanel::logHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{

    // instance->appendPlainText(finalMsg);
    if (!instance)
        return;
    // doing this since the log handler can be called from any thread,
    // so we need the invocation to be queued to the main thread where the LogPanel lives.
    QMetaObject::invokeMethod(instance, "appendLog", Qt::QueuedConnection, Q_ARG(QtMsgType, type),
                              Q_ARG(QString, msg));
}

void LogPanel::appendLog(QtMsgType type, const QString &msg)
{
    QString level;
    QTextCharFormat format;

    switch (type)
    {
    case QtDebugMsg:
        level = "DEBUG";
        break;
    case QtInfoMsg:
        level = "INFO";
        break;
    case QtWarningMsg:
        level = "WARNING";
        break;
    case QtCriticalMsg:
        level = "CRITICAL";
        break;
    case QtFatalMsg:
        level = "FATAL";
        break;
    }

    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");

    QString finalMsg = QString("[%1] [%2] %3").arg(time).arg(level).arg(msg);

    switch (type)
    {
    case QtDebugMsg:
        format.setForeground(Qt::gray);
        break;
    case QtInfoMsg:
        format.setForeground(Qt::white);
        break;
    case QtWarningMsg:
        format.setForeground(Qt::yellow);
        break;
    case QtCriticalMsg:
        format.setForeground(Qt::red);
        break;
    case QtFatalMsg:
        format.setForeground(Qt::red);
        format.setFontWeight(QFont::Bold);
        break;
    }

    QString text = finalMsg + "\n";

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text, format);

    setTextCursor(cursor);
}
} // namespace Kites