#include "ui/kites_editor.h"
#include <QFontDatabase>
#include <QPainter>
#include <QTextBlock>


namespace Kites
{
KitesEditor::KitesEditor(QWidget *parent)
    : QPlainTextEdit(parent), m_lineNumberArea(new LineNumberArea(this))
{
    connect(this, &KitesEditor::blockCountChanged, this, &KitesEditor::updateLineNumberAreaWidth);
    connect(this, &KitesEditor::updateRequest, this, &KitesEditor::updateLineNumberArea);
    connect(this, &KitesEditor::cursorPositionChanged, this, &KitesEditor::highlightCurrentLine);

    int id = QFontDatabase::addApplicationFont(":/fonts/Monaco.ttf");
    QString family = QFontDatabase::applicationFontFamilies(id).at(0);
    setFont(QFont(family, 11));

    setTabStopDistance(4);
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void KitesEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void KitesEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

int KitesEditor::lineNumberAreaWidth()
{
    int digits = QString::number(qMax(1, blockCount())).length();
    return 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void KitesEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
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
            // if (m_isTextEditor && std::find(m_breakPoints.begin(), m_breakPoints.end(), blockNumber + 1) != m_breakPoints.end()) 
            // {
            //     painter.setBrush(Qt::red);
            //     painter.setPen(Qt::NoPen);
            //     int radius = 8;
            //     painter.drawEllipse(2, top + (fontMetrics().height() - radius) / 2, radius, radius);
            // }
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void KitesEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::gray).lighter(160);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}


void KitesEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}
}// namespace Kites 