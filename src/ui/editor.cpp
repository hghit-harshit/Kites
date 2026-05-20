#include "ui/editor.h"
#include <QAbstractItemView> // this is needed for the auto completer popup
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar> // same reason as above
#include <QTextBlock>
#include <QToolTip>
#include <set>

// #include "ui/linenumberarea.h"
namespace Kites
{
Editor::Editor(QWidget *parent, bool isTextEditor)
    : QPlainTextEdit(parent), m_isTextEditor(isTextEditor)
//: m_LinesToHighlight({{".",1}})
{
    m_breakpointInteractionEnabled = isTextEditor;
    setMouseTracking(true);
    m_syntaxHighlighter = new SyntaxHighlighter(this->document());
    int id = QFontDatabase::addApplicationFont(":/fonts/Monaco.ttf");
    QString family = QFontDatabase::applicationFontFamilies(id).at(0);

    QFont font(family);
    font.setPointSize(11);
    font.setFixedPitch(true); // monospaced
    setFont(font);

    const int tapspace = 4;
    setTabStopDistance(tapspace * fontMetrics().horizontalAdvance(' '));

    if (!m_isTextEditor)
        return;

    connect(this, &QPlainTextEdit::blockCountChanged, this, [this](int /* newBlockCount */)
            { setViewportMargins(lineNumberAreaWidth(), 0, 0, 0); });

    connect(this, &QPlainTextEdit::updateRequest, this,
            [this](const QRect &rect, int dy)
            {
                if (dy)
                    m_lineNumberArea->scroll(0, dy);
                else
                    m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

                if (rect.contains(viewport()->rect()))
                    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
            });
    m_lineNumberArea = new LineNumberArea(this);
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);

    //---------Setting up auto completer---------
    QStringList words = {"harshit"};
    m_autoCompleter = new QCompleter(words, this);
    m_autoCompleter->setWidget(this);

    connect(m_autoCompleter, QOverload<const QString &>::of(&QCompleter::activated), this,
            &Editor::insertCompletion);
}

std::vector<uint64_t> Editor::getBreakpoints() const
{
    return m_breakPoints;
}

void Editor::setBreakpoints(const std::vector<uint64_t> &breakpoints)
{
    m_breakPoints = breakpoints;
    update();
}

void Editor::setBreakpointInteractionEnabled(bool enabled)
{
    m_breakpointInteractionEnabled = enabled;
}

void Editor::clearHighlights()
{
    m_LinesToHighlight.clear();
    viewport()->update(); // Trigger a repaint to remove highlights
}

void Editor::resetErrors()
{
    m_errorMessages.clear();
}

void Editor::setLinesToHighlight(const QVariantMap &linesToHighlight)
{
    m_LinesToHighlight = linesToHighlight;
    viewport()->update(); // Trigger a repaint to show the highlights
}

void Editor::insertCompletion(const QString &completion)
{
    QTextCursor tc = textCursor();
    int extra = completion.length() - m_autoCompleter->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));
    setTextCursor(tc);
}

QString Editor::textUnderCursor()
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void Editor::keyPressEvent(QKeyEvent *event)
{
    if (m_autoCompleter && m_autoCompleter->popup()->isVisible())
    {
        switch (event->key())
        {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            event->ignore();
            return;
        default:
            break;
        }
    }

    QPlainTextEdit::keyPressEvent(event);

    if (!m_autoCompleter)
        return;

    QString prefix = textUnderCursor();
    if (prefix.length() < 1)
    {
        m_autoCompleter->popup()->hide();
        return;
    }

    if (prefix != m_autoCompleter->completionPrefix())
    {
        m_autoCompleter->setCompletionPrefix(prefix);
        m_autoCompleter->popup()->setCurrentIndex(m_autoCompleter->completionModel()->index(0, 0));
    }

    QRect cr = cursorRect();
    cr.setWidth(m_autoCompleter->popup()->sizeHintForColumn(0) +
                m_autoCompleter->popup()->verticalScrollBar()->sizeHint().width());

    m_autoCompleter->complete(cr);
}

void Editor::paintEvent(QPaintEvent *event)
{
    std::set<int> drawnLines;
    QPlainTextEdit::paintEvent(event);

    QPainter painter(viewport());

    painter.setRenderHint(QPainter::Antialiasing);

    for (const auto &key : m_LinesToHighlight.keys())
    {
        int lineNumber = m_LinesToHighlight.value(key).toInt();
        if (lineNumber < 0 || lineNumber >= this->blockCount() ||
            drawnLines.find(lineNumber) != drawnLines.end())
        {
            continue;
        }
        drawnLines.insert(lineNumber);
        QTextBlock block =
            this->document()->findBlockByNumber(lineNumber - 1); // -1 for 0-based index

        if (block.isValid())
        {
            QRectF blockRect = this->blockBoundingGeometry(block).translated(this->contentOffset());

            QLinearGradient gradient(blockRect.topLeft(), blockRect.topRight());
            gradient.setColorAt(0.0, QColor(255, 255, 255, 0)); // white
            gradient.setColorAt(1.0, QColor(255, 0, 0, 100));   // red // Semi-transparent orange
            painter.fillRect(blockRect, gradient);

            QRectF textRect = blockRect.adjusted(0, 0, 0, -1);
            textRect.setWidth(viewport()->width() - 10);

            painter.setPen(Qt::black);
            painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, key);
        }
    }

    // we clear the lines to highlight after painting
    // otherwise the cursor will keep jumping to last highlighted line on every repaint
    // Now highlight the specified lines with gradient and draw messages
}

int Editor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10)
    {
        max /= 10;
        ++digits;
    }
    int space = 15 + fontMetrics().horizontalAdvance(QLatin1Char('1')) * digits;
    return space; // extra padding
}

void Editor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    if (!m_lineNumberArea)
        return;
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void Editor::lineNumberAreapaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1); // +1 for 1-based line numbers
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberAreaWidth() - 5, fontMetrics().height(),
                             Qt::AlignRight, number);

            // Draw breakpoint indicator if exists
            if (m_isTextEditor && std::find(m_breakPoints.begin(), m_breakPoints.end(),
                                            blockNumber + 1) != m_breakPoints.end())
            {
                painter.setBrush(Qt::red);
                painter.setPen(Qt::NoPen);
                int radius = 8;
                painter.drawEllipse(2, top + (fontMetrics().height() - radius) / 2, radius, radius);
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void Editor::lineNumberAreaMousePressEvent(QMouseEvent *event)
{
    if (!m_isTextEditor || !m_breakpointInteractionEnabled)
        return;

    auto toggleBreakpointAtLine = [this, event]()
    {
        QPoint clickPos = viewport()->mapFromGlobal(event->globalPosition().toPoint());
        QTextCursor cursor = cursorForPosition(clickPos);

        QTextBlock block = cursor.block();

        // check if click is within the block's vertical bounds
        // cause cusorForPosition returns nearest line if clicked below text
        if (clickPos.y() >= blockBoundingGeometry(block).translated(contentOffset()).top() &&
            clickPos.y() <= blockBoundingGeometry(block).translated(contentOffset()).bottom())
        {
            int lineNumber = block.blockNumber() + 1; // +1 for 1-based line numbers

            auto it = std::find(m_breakPoints.begin(), m_breakPoints.end(), lineNumber);
            if (it != m_breakPoints.end())
            {
                m_breakPoints.erase(it);
            }
            else
            {
                m_breakPoints.push_back(lineNumber);
            }
            update(); // Trigger a repaint to show/hide the breakpoint indicator
        }
    };

    if (event->button() == Qt::LeftButton && event->pos().x() < lineNumberAreaWidth())
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
            m_breakPoints.clear();
            update(); // Trigger a repaint to remove all breakpoint indicators
        }
        return; // Prevent base class selection
    }
}

void Editor::lineNumberAreaMouseMoveEvent(QMouseEvent *event)
{
    // Future implementation for mouse move events in the line number area
    QPlainTextEdit::mouseMoveEvent(event);
}

bool Editor::event(QEvent *event)
{
    // Check if the event is a tooltip request
    if (event->type() == QEvent::ToolTip)
    {
        // Cast the event to a QHelpEvent to get mouse coordinates
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);

        QTextCursor cursor = cursorForPosition(helpEvent->pos());
        QTextBlock block = cursor.block();
        // int textHeight = document()->size().height();
        QRectF blockRect = blockBoundingGeometry(block);
        blockRect.translate(contentOffset());

        // Doing this check to see if the mouse is actually over the text line
        // other wise if error is on last line and mouse is below text it still shows tooltip
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
        }
        else
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

void Editor::setErrorMessage(int line, const QString &message)
{
    m_errorMessages.emplace(line, message);
}
} // namespace Kites
