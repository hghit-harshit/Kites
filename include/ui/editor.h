#pragma once
#include <QPlainTextEdit>
#include <map>
#include <QEvent>
#include "ui/syntax_highlighter.h"
namespace Kites
{
class Editor : public QPlainTextEdit
{
    public :
        Editor(QWidget* parent = nullptr);
        void setErrorMessage(int line, const QString& message);
        void resetErrors();
    protected:
    //void mouseMoveEvent(QMouseEvent* event) override;
    bool event(QEvent *event) override;
    private : 
    std::map<int,QString> m_errorMessages;
    SyntaxHighlighter* m_syntaxHighlighter;
};

}// namespcae Kites