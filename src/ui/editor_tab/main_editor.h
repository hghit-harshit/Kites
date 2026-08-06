#pragma once
#include "ui/common/syntax_highlighter.h"
#include "ui/common/kites_editor.h"
#include <QCompleter>
#include <QEvent>
#include <QPlainTextEdit>
#include <QVariantMap>
#include <map>

namespace Kites
{
class Editor : public KitesEditor
{
    friend class BreakPointGutterColumn;
public:
    Editor(QWidget *parent = nullptr);
    void setErrorMessage(int line, const QString &message);
    void resetErrors();
    void setLinesToHighlight(const std::vector<std::pair<int,std::string>> &linesToHighlight);

    // these fuctions are for the line number
    // and breakpoint area
    // int breakpointAreaWidth() const;
    // void breakpointAreaPaintEvent(QPaintEvent *event);
    // void breakpointAreaMousePressEvent(QMouseEvent *event);

    std::vector<uint64_t> getBreakpoints() const;
    void setBreakpoints(const std::vector<uint64_t> &breakpoints);
    void setBreakpointInteractionEnabled(bool enabled);
    void clearHighlights();


protected:
    // void mouseMoveEvent(QMouseEvent* event) override;
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    // void resizeEvent(QResizeEvent *event) override;

private slots:
    void insertCompletion(const QString &completion);

private:
    // int leftViewMargin() const override;
    QString textUnderCursor() const;

    QCompleter *m_autoCompleter = nullptr;

    std::map<int, QString> m_errorMessages;
    SyntaxHighlighter *m_syntaxHighlighter;
    std::vector<std::pair<int, std::string>> m_LinesToHighlight;
    std::vector<uint64_t> m_breakPoints; // to pass to the vm
    // QSet<int> m_breakPointsSet; // for quick lookup
    // int m_gutterWidth = 30;
    bool m_isTextEditor; // to differentiate between text editor and disassembly viewer
    bool m_breakpointInteractionEnabled = true;
};
} // namespace Kites
