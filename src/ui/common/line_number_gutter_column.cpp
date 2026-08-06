#include "line_number_gutter_column.h"
#include "kites_editor.h"
#include "ui/theme/theme_manager.h"
#include <QPainter>
namespace Kites
{
LineNumberGutterColumn::LineNumberGutterColumn(KitesEditor* editor)
    : GutterColumn(editor)
{
    connect(&ThemeManager::getInstance(), &ThemeManager::editorThemeChangedSignal, this,
            [this](const QString &) { update(); });
    connect(editor, &KitesEditor::cursorPositionChanged, this,
            [this]() { update(); });
}

QSize LineNumberGutterColumn::sizeHint() const
{
    int digits = QString::number(m_editor->blockCount()).length();
    return QSize(12 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits, 0);
}

void LineNumberGutterColumn::paintEvent(QPaintEvent *event)
{
    // sits flush with the editor (Zed-style): same background, muted numbers,
    // full-strength number on the cursor line
    const ThemeManager &themeManager = ThemeManager::getInstance();
    QPainter painter(this);
    painter.fillRect(event->rect(), themeManager.getEditorBackgroundColor());

    const int currentLine = m_editor->textCursor().blockNumber();

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = blockTop(block);
    int bottom = top + blockHeight(block);

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1); // +1 for 1-based line numbers
            painter.setPen(blockNumber == currentLine
                               ? themeManager.getEditorForegroundColor()
                               : themeManager.getEditorMutedForegroundColor());
            painter.drawText(0, top, width() - 6, fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + blockHeight(block);
        ++blockNumber;
    }
}
}//namespace Kites
