#pragma once
#include "ui/common/syntax_highlighter.h"
#include <QCompleter>
#include <QEvent>
#include <QPlainTextEdit>
#include <QVariantMap>
#include <map>

namespace Kites
{

class LineNumberArea; // forward declaration
class Editor : public QPlainTextEdit
{
public:
    Editor(QWidget *parent = nullptr, bool isTextEditor = true);
    void setErrorMessage(int line, const QString &message);
    void resetErrors();
    void setLinesToHighlight(const std::vector<std::pair<int,std::string>> &linesToHighlight);

    // these fuctions are for the line number
    // and breakpoint area
    int lineNumberAreaWidth();
    void lineNumberAreapaintEvent(QPaintEvent *event);
    void lineNumberAreaMousePressEvent(QMouseEvent *event);
    void lineNumberAreaMouseMoveEvent(QMouseEvent *event);

    std::vector<uint64_t> getBreakpoints() const;
    void setBreakpoints(const std::vector<uint64_t> &breakpoints);
    void setBreakpointInteractionEnabled(bool enabled);
    void clearHighlights();

    void insertCompletion(const QString &completion);

protected:
    // void mouseMoveEvent(QMouseEvent* event) override;
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QString textUnderCursor();

    QCompleter *m_autoCompleter = nullptr;

    std::map<int, QString> m_errorMessages;
    SyntaxHighlighter *m_syntaxHighlighter;
    std::vector<std::pair<int, std::string>> m_LinesToHighlight;
    std::vector<uint64_t> m_breakPoints; // to pass to the vm
    // QSet<int> m_breakPointsSet; // for quick lookup
    // int m_gutterWidth = 30;
    bool m_isTextEditor; // to differentiate between text editor and disassembly viewer
    bool m_breakpointInteractionEnabled = true;
    LineNumberArea *m_lineNumberArea = nullptr;
};

class DisassemblyEditor : public Editor
{
public:
    DisassemblyEditor(QWidget *parent = nullptr) : Editor(parent, false)
    {
        setReadOnly(true);
    }
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(Editor *editor) : QWidget(editor), m_editor(editor)
    {
    }
    QSize sizeHint() const override
    {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_editor->lineNumberAreapaintEvent(event);
    }
    void mousePressEvent(QMouseEvent *event) override
    {
        m_editor->lineNumberAreaMousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        m_editor->lineNumberAreaMouseMoveEvent(event);
    }

private:
    Editor *m_editor;
};

} // namespace Kites
