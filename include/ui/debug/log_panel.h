#pragma once
#include <QPlainTextEdit>

namespace Kites
{

/**
 * @brief This is just a plain text edit that will help in debuggin by showing
 * debug, info, warning, critical and fatal messages in the application.
 * 
 */
class LogPanel : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit LogPanel(QWidget *parent = nullptr);
    ~LogPanel() override;

    static void logHandler(QtMsgType type, const QMessageLogContext&, const QString& msg);
public slots:
    void appendLog(QtMsgType type, const QString& msg);
private:
    static LogPanel* instance;
    
};

}//namespace Kites
