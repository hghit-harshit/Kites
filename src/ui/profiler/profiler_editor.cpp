#include "profiler_editor.h"
#include <QFontDatabase>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextEdit>


namespace Kites
{

ProfilerEditor::ProfilerEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    m_syntaxHighlighter = new SyntaxHighlighter(this->document());

    int id = QFontDatabase::addApplicationFont(":/fonts/Monaco.ttf");
    QString family;

    if (id != -1)
    {
        auto families = QFontDatabase::applicationFontFamilies(id);
        if (!families.isEmpty())
            family = families.at(0);
    }

    if (family.isEmpty())
        family = "Courier"; // fallback
    QFont font(family);
    font.setPointSize(11);
    font.setFixedPitch(true); // monospaced
    setFont(font);

    setReadOnly(true);
    setLineWrapMode(QPlainTextEdit::NoWrap);

    m_lineNumberArea      = new ProfilerEditorHelpers::LineNumberArea(this);
    m_ExecutionCountArea  = new ProfilerEditorHelpers::ExecutionCountArea(this);
    m_instructionTypeArea = new ProfilerEditorHelpers::InstructionTypeArea(this);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &ProfilerEditor::updateAreaWidths);
    connect(this, &QPlainTextEdit::updateRequest, this, &ProfilerEditor::updateAreas);
    updateAreaWidths();
}

void ProfilerEditor::setExecutionCount(const std::map<int, int> &hitCounts)
{
    m_line_number_execution_count_mapping = hitCounts;
    m_maxExecutionCount = 0;
    for (const auto &pair : m_line_number_execution_count_mapping)
    {
        m_maxExecutionCount = std::max(m_maxExecutionCount, pair.second);
    }

    QList<QTextEdit::ExtraSelection> selections;
    for (const auto &pair : m_line_number_execution_count_mapping)
    {
        const int lineNumber = pair.first;
        const int hits = pair.second;
        if (hits <= 0 || lineNumber <= 0)
        {
            continue;
        }

        QTextBlock block = document()->findBlockByNumber(lineNumber - 1);
        if (!block.isValid())
        {
            continue;
        }

        QTextEdit::ExtraSelection selection;
        selection.cursor = QTextCursor(block);
        selection.cursor.clearSelection();
        selection.format.setBackground(heatBackgroundColor(hits));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selections.append(selection);
    }
    setExtraSelections(selections);

    m_lineNumberArea->update();
    m_ExecutionCountArea->update();
    m_instructionTypeArea->update();
}

void ProfilerEditor::setInstructionTypes(const std::map<int, std::string> &instructionTypes)
{
    m_instructionTypes = instructionTypes;
    m_instructionTypeArea->update();
}

void ProfilerEditor::clearExecutionCount()
{
    m_line_number_execution_count_mapping.clear();
    m_maxExecutionCount = 0;
    setExtraSelections({});
    m_lineNumberArea->update();
    m_ExecutionCountArea->update();
    m_instructionTypeArea->update();
}

int ProfilerEditor::lineNumberAreaWidth() const
{
    int digits = QString::number(qMax(1, blockCount())).length();
    return fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

int ProfilerEditor::countAreaWidth() const
{
    int digits = m_maxExecutionCount > 0 ? QString::number(m_maxExecutionCount).length() : 1;
    return fontMetrics().horizontalAdvance(QLatin1Char('9')) * (digits + 1) + 10;
}

int ProfilerEditor::typeAreaWidth() const
{
    // Estimate width for instruction type display (e.g., "R-Type", "Pseudo", "F/D-R4")
    return fontMetrics().horizontalAdvance(QLatin1String("R-Type")) + 10;
}

void ProfilerEditor::updateAreaWidths()
{
    setViewportMargins(lineNumberAreaWidth(), 0, countAreaWidth() + typeAreaWidth(), 0);
}

void ProfilerEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));

    int scrollbarWidth = verticalScrollBar()->isVisible() ? verticalScrollBar()->width() : 0;

    // Position count area
    m_ExecutionCountArea->setGeometry(QRect(cr.right() - countAreaWidth() - typeAreaWidth() - scrollbarWidth,
                                           cr.top(), countAreaWidth(), cr.height()));

    // Position type area
    m_instructionTypeArea->setGeometry(QRect(cr.right() - typeAreaWidth() - scrollbarWidth, cr.top(),
                                           typeAreaWidth(), cr.height()));
}

void ProfilerEditor::updateAreas(const QRect &rect, int dy)
{
    if (dy)
    {
        m_lineNumberArea->scroll(0, dy);
        m_ExecutionCountArea->scroll(0, dy);
        m_instructionTypeArea->scroll(0, dy);
    }
    else
    {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
        m_ExecutionCountArea->update(0, rect.y(), m_ExecutionCountArea->width(), rect.height());
        m_instructionTypeArea->update(0, rect.y(), m_instructionTypeArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect()))
    {
        updateAreaWidths();
    }
}

void ProfilerEditor::paintLineNumberArea(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), palette().color(QPalette::AlternateBase));

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
            painter.drawText(0, top, lineNumberAreaWidth() - 2, fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void ProfilerEditor::paintCountArea(QPaintEvent *event)
{
    QPainter painter(m_ExecutionCountArea);
    painter.fillRect(event->rect(), palette().color(QPalette::Base));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            const int lineNumber = blockNumber + 1;
            int hits;
            hits = m_line_number_execution_count_mapping.count(lineNumber) > 0 ? 
            m_line_number_execution_count_mapping[lineNumber] : 0;

            if (hits > 0)
            {
                painter.setPen(Qt::black);
                painter.drawText(0, top, countAreaWidth() - 5, fontMetrics().height(),
                                 Qt::AlignLeft, QString::number(hits));
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void ProfilerEditor::paintTypeArea(QPaintEvent *event)
{
    QPainter painter(m_instructionTypeArea);
    painter.fillRect(event->rect(), palette().color(QPalette::Base));
    const QColor textColor = palette().color(QPalette::Text);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            const int lineNumber = blockNumber + 1;
            if (m_instructionTypes.count(lineNumber) > 0)
            {
                const std::string &instrType = m_instructionTypes.at(lineNumber);
                painter.setPen(textColor);
                painter.drawText(5, top, typeAreaWidth() - 10, fontMetrics().height(),
                                 Qt::AlignLeft, QString::fromStdString(instrType));
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

QColor ProfilerEditor::heatColor(int hitCount) const
{
    if (m_maxExecutionCount == 0 || hitCount == 0)
        return Qt::transparent;

    double t = (double)hitCount / m_maxExecutionCount;
    if (t > 0.80)
        return QColor("#D85A30");
    if (t > 0.50)
        return QColor("#EF9F27");
    if (t > 0.20)
        return QColor("#FAC775");
    return QColor("#FAEEDA");
}

QColor ProfilerEditor::heatBackgroundColor(int hitCount) const
{
    if (m_maxExecutionCount == 0 || hitCount == 0)
        return Qt::transparent;

    if (m_maxExecutionCount == 0 || hitCount == 0)
        return Qt::transparent;
    double t = (double)hitCount / m_maxExecutionCount;
    int alpha = static_cast<int>(12 + t * 40);
    return QColor(216, 90, 48, alpha);
}
} // namespace Kites
