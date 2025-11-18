#include "ui/editor.h"
#include <QMouseEvent>
#include <QTextBlock>
#include <QToolTip>
#include <QPainter>
#include <set>
namespace Kites
{
Editor::Editor(QWidget* parent):QPlainTextEdit(parent)
//:m_LinesToHighlight({{".",1}})
{
    setMouseTracking(true);
    m_syntaxHighlighter = new SyntaxHighlighter(this->document());
}

void Editor::resetErrors()
{
    m_errorMessages.clear();
}

void Editor::setLinesToHighlight(const QVariantMap& linesToHighlight)
{
    m_LinesToHighlight = linesToHighlight;
    viewport()->update(); // Trigger a repaint to show the highlights
}

void Editor::paintEvent(QPaintEvent *event)
{
    std::set<int> drawnLines;
    QPlainTextEdit::paintEvent(event);

    QPainter painter(viewport());
    
    painter.setRenderHint(QPainter::Antialiasing);

    for(const auto& key : m_LinesToHighlight.keys())
    {
        int lineNumber = m_LinesToHighlight.value(key).toInt();
        if(lineNumber < 0 || lineNumber >= this->blockCount() || drawnLines.find(lineNumber) != drawnLines.end())
        { continue; }
        drawnLines.insert(lineNumber);  
        QTextBlock block = this->document()->findBlockByNumber(lineNumber-1); // -1 for 0-based index

        if (block.isValid()) 
        {
            QRectF blockRect = this->blockBoundingGeometry(block).translated(this->contentOffset());
            
            QLinearGradient gradient(blockRect.topLeft(), blockRect.topRight());
            gradient.setColorAt(0.0, QColor(255, 255, 255,0));  // white
            gradient.setColorAt(1.0, QColor(255,0,0,100));  // red // Semi-transparent orange
            painter.fillRect(blockRect, gradient);

            QRectF textRect = blockRect.adjusted(0, 0, 0, -1);
            textRect.setWidth(viewport()->width()-10);


            painter.setPen(Qt::black);
            painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, key);
        }
    }
}

bool Editor::event(QEvent *event)
{
    // Check if the event is a tooltip request
    if (event->type() == QEvent::ToolTip) 
    {
        // Cast the event to a QHelpEvent to get mouse coordinates
        QHelpEvent *helpEvent = static_cast<QHelpEvent*>(event);

        QTextCursor cursor = cursorForPosition(helpEvent->pos());
        QTextBlock block = cursor.block();
        //int textHeight = document()->size().height();
        QRectF blockRect = blockBoundingGeometry(block);
        blockRect.translate(contentOffset());

       //Doing this check to see if the mouse is actually over the text line
       //other wise if error is on last line and mouse is below text it still shows tooltip
        if (helpEvent->pos().y() < blockRect.top() || helpEvent->pos().y() > blockRect.bottom()) 
        {
            // The mouse is below (or above) the actual text line, likely in the empty space.
            QToolTip::hideText();
            event->ignore(); 
            return true; 
        }
      
        
        int lineNumber = block.blockNumber();

        // If this line has a tooltip, show it
        // (Using .count() is a common and clean way to check for key existence)
        if (m_errorMessages.find(lineNumber) != m_errorMessages.end()) 
        {
            // Show our custom error tooltip
            QToolTip::showText(helpEvent->globalPos(), m_errorMessages.at(lineNumber), this);
            return true; // We handled this event!
        } else 
        {
            // No error for this line, hide any active tooltip
            QToolTip::hideText();
            event->ignore();
            // Don't return true, let the base class handle it
            // (e.g., to show the widget's default tooltip if one is set)
        }
        return true;
    }
    
    // Pass all other events (including mouseMoveEvent) to the base class
    return QPlainTextEdit::event(event);
}

void Editor::setErrorMessage(int line, const QString& message)
{
    m_errorMessages.emplace(line,message);
}
}
