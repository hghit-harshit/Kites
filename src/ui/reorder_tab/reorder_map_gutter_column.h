#pragma once
#include "reorder_editor.h"
#include "ui/common/gutter_column.h"
#include "ui/theme/theme_manager.h"
#include <QPainter>
#include <algorithm>

namespace Kites
{
// Right-hand gutter: "from -> to" for each line. Unmoved instructions are muted,
// moved ones are drawn at full strength.
class ReorderMapGutterColumn : public GutterColumn
{
  public:
    explicit ReorderMapGutterColumn(ReorderEditor *editor) : GutterColumn(editor), m_reorderEditor(editor)
    {
    }

    QSize sizeHint() const override
    {
        const int n = static_cast<int>(m_reorderEditor->m_origIndex.size());
        const int digits = QString::number(std::max(n, 1)).length();
        const QString widest = QString("#%1 %2 #%1").arg(QString(digits, QLatin1Char('9')), arrow());
        return QSize(fontMetrics().horizontalAdvance(widest) + 14, 0);
    }

  protected:
    void paintEvent(QPaintEvent *event) override
    {
        const auto &themeManager = ThemeManager::getInstance();
        QPainter painter(this);
        painter.fillRect(event->rect(), themeManager.getEditorBackgroundColor());

        const std::vector<int> &orig = m_reorderEditor->m_origIndex;

        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = blockTop(block);
        int bottom = top + blockHeight(block);

        while (block.isValid() && top <= event->rect().bottom())
        {
            if (block.isVisible() && bottom >= event->rect().top() &&
                blockNumber < static_cast<int>(orig.size()))
            {
                const int from = orig[blockNumber];
                const int to = blockNumber;
                painter.setPen(from == to ? themeManager.getEditorMutedForegroundColor()
                                          : themeManager.getEditorForegroundColor());
                painter.drawText(7, top, width() - 14, fontMetrics().height(), Qt::AlignLeft,
                                 QString("#%1 %2 #%3").arg(from).arg(arrow()).arg(to));
            }

            block = block.next();
            top = bottom;
            bottom = top + blockHeight(block);
            ++blockNumber;
        }
    }

  private:
    static QString arrow()
    {
        return QString(QChar(0x2192)); // right arrow
    }
    ReorderEditor *m_reorderEditor;
};
} // namespace Kites
