#include "reorder_editor.h"
#include "reorder_map_gutter_column.h"
#include "ui/common/line_number_gutter_column.h"
#include "ui/common/syntax_highlighter.h"
#include <QResizeEvent>
#include <QTextBlock>
#include <QTextEdit>

namespace Kites
{
ReorderEditor::ReorderEditor(QWidget *parent) : KitesEditor(parent)
{
    m_syntaxHighlighter = new SyntaxHighlighter(document());
    setReadOnly(true);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    addLeftGutterColumn(new LineNumberGutterColumn(this));
    addRightGutterColumn(new ReorderMapGutterColumn(this));
    relayoutGutters();
}

void ReorderEditor::setReordered(const QString &text, const std::vector<int> &origIndex)
{
    setPlainText(text);
    m_origIndex = origIndex;
    applyMovedHighlights();
    relayoutGutters(); // right gutter width depends on how many digits the indices need
}

void ReorderEditor::clearReordered()
{
    clear();
    m_origIndex.clear();
    setExtraSelections({});
    relayoutGutters();
}

void ReorderEditor::applyMovedHighlights()
{
    // tint every line whose instruction moved, same mechanism the profiler uses for its heatmap
    QList<QTextEdit::ExtraSelection> selections;
    for (size_t k = 0; k < m_origIndex.size(); ++k)
    {
        if (m_origIndex[k] == static_cast<int>(k))
            continue;
        QTextBlock block = document()->findBlockByNumber(static_cast<int>(k));
        if (!block.isValid())
            continue;
        QTextEdit::ExtraSelection selection;
        selection.cursor = QTextCursor(block);
        selection.cursor.clearSelection();
        selection.format.setBackground(QColor(80, 140, 230, 40));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selections.append(selection);
    }
    setExtraSelections(selections);
}

void ReorderEditor::relayoutGutters()
{
    updateViewPortMargins();
    // KitesEditor only positions gutters in resizeEvent, so re-run it after widths change
    QResizeEvent e(size(), size());
    resizeEvent(&e);
    viewport()->update();
}
} // namespace Kites
