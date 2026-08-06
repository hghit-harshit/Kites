#pragma once
#include "ui/common/gutter_column.h"
#include "ui/theme/theme_manager.h"
#include "main_editor.h"
#include <QPainter>
#include <QMenu>
namespace Kites
{
class BreakPointGutterColumn : public GutterColumn
{
    Q_OBJECT
public:
    explicit BreakPointGutterColumn(Editor* editor = nullptr)
    :GutterColumn(editor), m_mainEditor(editor)
    {
    }
    QSize sizeHint() const override
    {
        return QSize(15, 0); 
    }
protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(this);
        painter.fillRect(event->rect(),
                         ThemeManager::getInstance().getEditorBackgroundColor());

        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = blockTop(block);
        int bottom = top + blockHeight(block);

        while (block.isValid() && top <= event->rect().bottom())
        {
            if (block.isVisible() && bottom >= event->rect().top())
            {
                // Draw breakpoint indicator if exists
                if ( std::find(m_mainEditor->m_breakPoints.begin(), 
                m_mainEditor->m_breakPoints.end(),
                blockNumber + 1) != m_mainEditor->m_breakPoints.end())
                {
                    painter.setBrush(Qt::red);
                    painter.setPen(Qt::NoPen);
                    int radius = 8;
                    painter.drawEllipse(2, top + (fontMetrics().height() - radius) / 2, radius, radius);
                }
            }

            block = block.next();
            top = bottom;
            bottom = top + blockHeight(block);
            ++blockNumber;
        }
    }

    void mousePressEvent(QMouseEvent *event) override
    {

        auto toggleBreakpointAtLine = [this, event]()
        {
            QPoint clickPos = m_mainEditor->viewport()->mapFromGlobal(event->globalPosition().toPoint());
            QTextCursor cursor = m_mainEditor->cursorForPosition(clickPos);

            QTextBlock block = cursor.block();

            // check if click is within the block's vertical bounds
            // cause cusorForPosition returns nearest line if clicked below text
            if (clickPos.y() >= blockTop(block) &&
                clickPos.y() <= blockTop(block) + blockHeight(block))
            {
                int lineNumber = block.blockNumber() + 1; // +1 for 1-based line numbers
                auto it = std::find(m_mainEditor->m_breakPoints.begin(), m_mainEditor->m_breakPoints.end(), lineNumber);
                if (it != m_mainEditor->m_breakPoints.end())
                {
                    m_mainEditor->m_breakPoints.erase(it);
                }
                else
                {
                    m_mainEditor->m_breakPoints.push_back(lineNumber);
                }
                update(); // Trigger a repaint to show/hide the breakpoint indicator
            }
        };

        if (event->button() == Qt::LeftButton && event->pos().x() < width())
        {
            toggleBreakpointAtLine();
        }

        if (event->button() == Qt::RightButton)
        {
            QMenu menu(this);

            QAction *action1 = menu.addAction("Toggle Breakpoint");
            QAction *action2 = menu.addAction("Remove All Breakpoints");

            QAction *selectedAction = menu.exec(event->globalPosition().toPoint());
            if (selectedAction == action1)
            {
                toggleBreakpointAtLine();
            }
            else if (selectedAction == action2)
            {
                m_mainEditor->m_breakPoints.clear();
                update(); // Trigger a repaint to remove all breakpoint indicators
            }
            return; // Prevent base class selection
        }
        
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        // draw a faint circle on the line where the mouse is hovering
    }
private:
    Editor* m_mainEditor;

};
}//namespace Kites