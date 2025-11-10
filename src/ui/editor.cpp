#include "ui/editor.h"
#include <QMouseEvent>
#include <QTextBlock>
#include <QToolTip>
namespace Kites
{
Editor::Editor(QWidget* parent):QPlainTextEdit(parent)
{
    setMouseTracking(true);
    m_syntaxHighlighter = new SyntaxHighlighter(this->document());
}

void Editor::resetErrors()
{
    m_errorMessages.clear();
}

// void Editor::mouseMoveEvent(QMouseEvent* event)
// {
//     QTextCursor cursor = cursorForPosition(event->pos());
//     QTextBlock block = cursor.block();
//     int lineNumber = block.blockNumber();  // 0-based line index

//     // If this line has a tooltip, show it
//     if (m_errorMessages.find(lineNumber) != m_errorMessages.en   d()) {
//         QToolTip::showText(event->globalPos(), m_errorMessages[lineNumber], this);
//     } else {
//         QToolTip::hideText();
//     }

//     QPlainTextEdit::mouseMoveEvent(event);
// }

bool Editor::event(QEvent *event)
{
    // Check if the event is a tooltip request
    if (event->type() == QEvent::ToolTip) 
    {
        // Cast the event to a QHelpEvent to get mouse coordinates
        QHelpEvent *helpEvent = static_cast<QHelpEvent*>(event);

        // Use your existing logic to find the line number
        QTextCursor cursor = cursorForPosition(helpEvent->pos());
        QTextBlock block = cursor.block();
        int lineNumber = block.firstLineNumber();

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
