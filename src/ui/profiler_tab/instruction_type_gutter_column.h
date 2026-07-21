#pragma once
#include "ui/common/gutter_column.h"
#include "profiler_editor.h"
#include <QPainter>
namespace Kites
{
    class InstructionTypeGutterColumn : public GutterColumn
    {
    public:
        InstructionTypeGutterColumn(ProfilerEditor *editor) : GutterColumn(editor), m_profilerEditor(editor)
        {}

        QSize sizeHint() const override
        {
            // Estimate width for instruction type display (e.g., "R-Type", "Pseudo", "F/D-R4")
            return QSize(fontMetrics().horizontalAdvance(QLatin1String("R-Type")) + 10, 0);
        }

    protected:
        void paintEvent(QPaintEvent *event) override
        {
            QPainter painter(this);
            painter.fillRect(event->rect(), palette().color(QPalette::Base));
            const QColor textColor = palette().color(QPalette::Text);

            QTextBlock block = firstVisibleBlock();
            int blockNumber = block.blockNumber();
            int top = blockTop(block);
            int bottom = top + blockHeight(block);

            while (block.isValid() && top <= event->rect().bottom())
            {
                if (block.isVisible() && bottom >= event->rect().top())
                {
                    const int lineNumber = blockNumber + 1;
                    if (m_profilerEditor->m_lineNumberToInstructionType.count(lineNumber) > 0)
                    {
                        const std::string &instrType = m_profilerEditor->m_lineNumberToInstructionType.at(lineNumber);
                        painter.setPen(textColor);
                        painter.drawText(5, top, width() - 10, fontMetrics().height(),
                                         Qt::AlignLeft, QString::fromStdString(instrType));
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
}