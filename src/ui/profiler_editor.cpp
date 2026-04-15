#include "ui/profiler_editor.h"
#include <QFontDatabase>
#include <QPainter>
#include <QResizeEvent>
#include <QFontDatabase>
#include <QScrollBar>
#include <QTextBlock>

namespace Kites 
{

ProfilerEditor::ProfilerEditor(QWidget* parent) : QPlainTextEdit(parent)
{
    int id = QFontDatabase::addApplicationFont(":/fonts/Monaco.ttf");
    QString family;

    if (id != -1) {
        auto families = QFontDatabase::applicationFontFamilies(id);
        if (!families.isEmpty())
            family = families.at(0);
    }

    if (family.isEmpty())
        family = "Courier"; // fallback
    QFont font(family);
    font.setPointSize(11);
    font.setFixedPitch(true);  // monospaced
    setFont(font);


    setReadOnly(true);
    setLineWrapMode(QPlainTextEdit::NoWrap);

    m_lineNumberArea = new ProfilerEditorHelpers::LineNumberArea(this);
    m_countArea = new ProfilerEditorHelpers::CountArea(this);

    // connect(this, &QPlainTextEdit::blockCountChanged, this, [this](int /* newBlockCount */) 
    // {
    //     setViewportMargins(LineNumberAreaWidth() + CountAreaWidth(), 0, 0, 0);
    // });
    // connect(this, &QPlainTextEdit::updateRequest, this, [this](const QRect &rect, int dy) 
    // {
    //     if (dy)
    //     {
    //         m_lineNumberArea->scroll(0, dy);
    //         m_countArea->scroll(0, dy);
    //     }
    //     else
    //     {
    //         m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    //         m_countArea->update(0, rect.y(), m_countArea->width(), rect.height());
    //     }

    //     if (rect.contains(viewport()->rect()))
    //         setViewportMargins(LineNumberAreaWidth() + CountAreaWidth(), 0, 0, 0);
    // });

    connect(this, &QPlainTextEdit::blockCountChanged,this, &ProfilerEditor::updateAreaWidths);
    connect(this, &QPlainTextEdit::updateRequest,this, &ProfilerEditor::updateAreas);
    updateAreaWidths();
}

void ProfilerEditor::setHitCount(const std::map<int,int>& hitCounts)
{
    m_hitCounts = hitCounts;
    m_maxHitCount = 0;
    for (const auto& pair : m_hitCounts)
    {
        m_maxHitCount = std::max(m_maxHitCount, pair.second);
    }
    m_lineNumberArea->update();
    m_countArea->update();
}

void ProfilerEditor::clearHitCount()
{
    m_hitCounts.clear();
    m_maxHitCount = 0;
    m_lineNumberArea->update();
    m_countArea->update();
}

int ProfilerEditor::lineNumberAreaWidth() const
{
    int digits = QString::number(qMax(1,blockCount())).length();
    return 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

int ProfilerEditor::countAreaWidth() const
{
    int digits = m_maxHitCount > 0 ? QString::number(m_maxHitCount).length() : 4;
    return fontMetrics().horizontalAdvance(QLatin1Char('9')) * (digits + 1) + 16;
}

void ProfilerEditor::updateAreaWidths()
{
    setViewportMargins(lineNumberAreaWidth(),0, countAreaWidth(), 0);
}

void ProfilerEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
    //m_countArea->setGeometry(QRect(cr.right() - countAreaWidth(), cr.top(), countAreaWidth(), cr.height()));

    int scrollbarWidth = verticalScrollBar()->isVisible()
                     ? verticalScrollBar()->width()
                     : 0;

    m_countArea->setGeometry(
        QRect(
            cr.right() - countAreaWidth() - scrollbarWidth,
            cr.top(),
            countAreaWidth(),
            cr.height()
        )
    );
}

void ProfilerEditor::updateAreas(const QRect &rect, int dy)
{
    if(dy)
    {
        m_lineNumberArea->scroll(0, dy);
        m_countArea->scroll(0, dy);
    }
    else
    {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
        m_countArea->update(0, rect.y(), m_countArea->width(), rect.height());
    }

    if(rect.contains(viewport()->rect()))
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

    while(block.isValid() && top <= event->rect().bottom())
    {
        if(block.isVisible() && bottom >= event->rect().top())
        {
            QString number = QString::number(blockNumber + 1); // +1 for 1-based line numbers
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberAreaWidth() - 5, fontMetrics().height(),
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
    QPainter painter(m_countArea);
    painter.fillRect(event->rect(), palette().color(QPalette::AlternateBase));

    constexpr int heatStripWidth = 4;
    const int textX = heatStripWidth + 4;
    const int textW = m_countArea->width() - textX - 6;

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    while(block.isValid() && top <= event->rect().bottom())
    {
        if(block.isVisible() && bottom >= event->rect().top())
        {
            const int lineNumber = blockNumber + 1;
            int hits;
            hits = m_hitCounts.count(lineNumber) > 0 ? m_hitCounts[lineNumber] : 0;
            
            if(hits > 0)
            {
               painter.fillRect(0, top, heatStripWidth, fontMetrics().height(), heatColor(hits));
                // int fullWidth = width();   // total widget width
                // painter.fillRect(0, top, fullWidth, fontMetrics().height(), heatColor(hits));
               double t = (double)hits / m_maxHitCount;
               //painter.setPen(heatBackgroundColor(hits));
               painter.setPen(Qt::black);
                // painter.drawText(textX, top, textW, fontMetrics().height(),
                //            Qt::AlignRight | Qt::AlignVCenter,
                //            QString::number(hits));

                painter.drawText(0, top, countAreaWidth() - 5, fontMetrics().height(),
                            Qt::AlignRight, QString::number(hits));
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
    if(m_maxHitCount == 0 || hitCount == 0)
        return Qt::transparent;

    double t = (double)hitCount / m_maxHitCount;
    if (t > 0.80) return QColor("#D85A30");
    if (t > 0.50) return QColor("#EF9F27");
    if (t > 0.20) return QColor("#FAC775");
    return QColor("#FAEEDA");
}

QColor ProfilerEditor::heatBackgroundColor(int hitCount) const
{
    if(m_maxHitCount == 0 || hitCount == 0)
        return Qt::transparent;

    // double t = (double)hitCount / m_maxHitCount;
    // if (t > 0.80) return QColor("#D85A30").lighter(150);
    // if (t > 0.50) return QColor("#EF9F27").lighter(150);
    // if (t > 0.20) return QColor("#FAC775").lighter(150);
    // return QColor("#FAEEDA").lighter(150);

    if (m_maxHitCount == 0 || hitCount == 0) return Qt::transparent;
    double t    = (double)hitCount / m_maxHitCount;
    int    alpha = static_cast<int>(12 + t * 40);
    return QColor(216, 90, 48, alpha);
}
}// namespace Kites