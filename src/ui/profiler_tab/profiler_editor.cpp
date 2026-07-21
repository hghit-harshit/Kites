#include "profiler_editor.h"
#include <QFontDatabase>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextEdit>
#include "ui/common/line_number_gutter_column.h"
#include "ui/common/syntax_highlighter.h"
#include "instruction_type_gutter_column.h"
#include "execution_count_gutter_column.h"
namespace Kites
{

ProfilerEditor::ProfilerEditor(QWidget *parent) : KitesEditor(parent)
{
    m_syntaxHighlighter = new SyntaxHighlighter(this->document());
    setReadOnly(true);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    addLeftGutterColumn(new LineNumberGutterColumn(this));
    //DO NOT CHANGE THE ORDER OF THESE TWO LINES BELOW. 
    addRightGutterColumn(new ExecutionCountGutterColumn(this));
    addRightGutterColumn(new InstructionTypeGutterColumn(this));
    updateViewPortMargins();
}

void ProfilerEditor::setExecutionCount(const std::map<int, int> &hitCounts)
{
    m_lineNumberToExecutionCounts = hitCounts;
    m_maxExecutionCount = 0;
    for (const auto &pair : m_lineNumberToExecutionCounts)
    {
        m_maxExecutionCount = std::max(m_maxExecutionCount, pair.second);
    }

    QList<QTextEdit::ExtraSelection> selections;
    for (const auto &pair : m_lineNumberToExecutionCounts)
    {
        const int lineNumber = pair.first;
        const int hits = pair.second;
        if (hits <= 0 || lineNumber <= 0)
        {
            continue;
        }

        QTextBlock block = document()->findBlockByNumber(lineNumber - 1);
        if (!block.isValid())
        {
            continue;
        }

        QTextEdit::ExtraSelection selection;
        selection.cursor = QTextCursor(block);
        selection.cursor.clearSelection();
        selection.format.setBackground(heatBackgroundColor(hits));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selections.append(selection);
    }
    setExtraSelections(selections);
    viewport()->update();
}

void ProfilerEditor::setInstructionTypes(const std::map<int, std::string> &instructionTypes)
{
    m_lineNumberToInstructionType = instructionTypes;
}

void ProfilerEditor::clearExecutionCount()
{
    m_lineNumberToExecutionCounts.clear();
    m_maxExecutionCount = 0;
    setExtraSelections({});
    viewport()->update();
}

QColor ProfilerEditor::heatColor(int hitCount) const
{
    if (m_maxExecutionCount == 0 || hitCount == 0)
        return Qt::transparent;

    double t = (double)hitCount / m_maxExecutionCount;
    if (t > 0.80)
        return QColor("#D85A30");
    if (t > 0.50)
        return QColor("#EF9F27");
    if (t > 0.20)
        return QColor("#FAC775");
    return QColor("#FAEEDA");
}

QColor ProfilerEditor::heatBackgroundColor(int hitCount) const
{
    if (m_maxExecutionCount == 0 || hitCount == 0)
        return Qt::transparent;

    if (m_maxExecutionCount == 0 || hitCount == 0)
        return Qt::transparent;
    double t = (double)hitCount / m_maxExecutionCount;
    int alpha = static_cast<int>(12 + t * 40);
    return QColor(216, 90, 48, alpha);
}
} // namespace Kites
