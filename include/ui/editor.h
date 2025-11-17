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
        void setLinesToHighlight(const QVariantMap& linesToHighlight);
    protected:
    //void mouseMoveEvent(QMouseEvent* event) override;
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent* event) override;
    private : 
    std::map<int,QString> m_errorMessages;
    SyntaxHighlighter* m_syntaxHighlighter;
    QVariantMap m_LinesToHighlight;
};

}// namespcae Kites