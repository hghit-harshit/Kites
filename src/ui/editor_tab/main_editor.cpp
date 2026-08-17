#include "main_editor.h"
#include "language_service/language_service.h"
#include <QAbstractItemView>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStringListModel>
#include <QTextBlock>
#include <QToolTip>
#include <set>
#include "ui/common/line_number_gutter_column.h"
#include "ui/theme/font_manager.h"
#include "break_point_gutter_column.h"
namespace Kites
{
namespace
{
bool isCompletionWordChar(QChar c)
{
    // default Qt word boundaries exclude '.'/'$', which real RISC-V syntax needs
    // (fadd.s, and labels can contain '.'/'$')
    return c.isLetterOrNumber() || c == '_' || c == '.' || c == '$';
}
} // namespace

Editor::Editor(QWidget *parent )
    : KitesEditor(parent)
{
    setMouseTracking(true);
    m_syntaxHighlighter = new SyntaxHighlighter(this->document());
    setFont(FontManager::getInstance().currentFont());

    const int tapspace = 4;
    setTabStopDistance(tapspace * fontMetrics().horizontalAdvance(' '));
    addLeftGutterColumn(new BreakPointGutterColumn(this));
    addLeftGutterColumn(new LineNumberGutterColumn(this));
    updateViewPortMargins();

    m_autoCompleter = new QCompleter(this);
    m_autoCompleter->setModel(new QStringListModel(m_autoCompleter));
    m_autoCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    m_autoCompleter->setCompletionMode(QCompleter::PopupCompletion);
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
    viewport()->update(); 
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

void Editor::setLinesToHighlight(const std::vector<std::pair<int, std::string>> &linesToHighlight)
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

    for (const auto [lineNumber, instructionStage] : m_LinesToHighlight)
    {
        if (lineNumber < 0 || lineNumber > this->blockCount() ||
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
            painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, instructionStage.c_str());
        }
    }

    // we clear the lines to highlight after painting
    // otherwise the cursor will keep jumping to last highlighted line on every repaint
    // Now highlight the specified lines with gradient and draw messages
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

QString Editor::textUnderCursor() const
{
    const QTextCursor cursor = textCursor();
    const QTextBlock block = cursor.block();
    const QString blockText = block.text();
    const int posInBlock = cursor.position() - block.position();

    int start = posInBlock;
    while (start > 0 && isCompletionWordChar(blockText[start - 1]))
        --start;
    int end = posInBlock;
    while (end < blockText.size() && isCompletionWordChar(blockText[end]))
        ++end;

    return blockText.mid(start, end - start);
}

void Editor::insertCompletion(const QString &completion)
{
    if (m_autoCompleter->widget() != this)
        return;

    QTextCursor cursor = textCursor();
    const int prefixLength = m_autoCompleter->completionPrefix().length();
    cursor.setPosition(cursor.position() - prefixLength);
    cursor.setPosition(cursor.position() + prefixLength, QTextCursor::KeepAnchor);
    cursor.insertText(completion);
    setTextCursor(cursor);
}

void Editor::keyPressEvent(QKeyEvent *event)
{
    if (m_autoCompleter && m_autoCompleter->popup()->isVisible())
    {
        // let the completer's own popup handle these - it has an event filter
        // installed on this widget that already intercepts most of them, this
        // guard covers the rest (matches Qt's own QCompleter usage pattern)
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

    KitesEditor::keyPressEvent(event);

    if (!m_autoCompleter)
        return;

    const QString prefix = textUnderCursor();
    if (prefix.isEmpty())
    {
        m_autoCompleter->popup()->hide();
        return;
    }

    const QStringList matches = LanguageService::completions(prefix, toPlainText());
    if (matches.isEmpty())
    {
        m_autoCompleter->popup()->hide();
        return;
    }

    auto *model = qobject_cast<QStringListModel *>(m_autoCompleter->model());
    model->setStringList(matches);

    m_autoCompleter->setCompletionPrefix(prefix);
    m_autoCompleter->popup()->setCurrentIndex(m_autoCompleter->completionModel()->index(0, 0));

    QRect completionRect = cursorRect();
    completionRect.setWidth(m_autoCompleter->popup()->sizeHintForColumn(0) +
                            m_autoCompleter->popup()->verticalScrollBar()->sizeHint().width());
    m_autoCompleter->complete(completionRect);
}
} // namespace Kites
