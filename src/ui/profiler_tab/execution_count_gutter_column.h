#pragma once
#include "ui/common/gutter_column.h"
#include "profiler_editor.h"
#include <QPainter>

namespace Kites
{
class ExecutionCountGutterColumn : public GutterColumn
{
public:
    ExecutionCountGutterColumn(ProfilerEditor *editor) : GutterColumn(editor), m_profilerEditor(editor)
    {}

    QSize sizeHint() const override
    {
        int digits = QString::number(m_profilerEditor->m_maxExecutionCount).length();
        return QSize((fontMetrics().horizontalAdvance(QLatin1Char('9')) * (digits + 1) + 10), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        painter.fillRect(event->rect(), palette().color(QPalette::Base));

        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = blockTop(block);
        int bottom = top + blockHeight(block);

        while (block.isValid() && top <= event->rect().bottom())
        {
            if (block.isVisible() && bottom >= event->rect().top())
            {
                const int lineNumber = blockNumber + 1;
                int hits;
                hits = m_profilerEditor->m_lineNumberToExecutionCounts.count(lineNumber) > 0 ? 
                m_profilerEditor->m_lineNumberToExecutionCounts[lineNumber] : 0;

                if (hits > 0)
                {
                    painter.setPen(Qt::black);
                    painter.drawText(0, top, width() - 5, fontMetrics().height(),
                                     Qt::AlignLeft, QString::number(hits));
                }
            }

            block = block.next();
            top = bottom;
            bottom = top + blockHeight(block);
            ++blockNumber;
        }
    }
private:
    ProfilerEditor *m_profilerEditor;
};
}//namespace Kites