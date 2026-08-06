#include "kites_editor.h"
#include "gutter_column.h"
#include "ui/theme/font_manager.h"
#include "ui/theme/theme_manager.h"
#include <QPainter>
#include <QTextBlock>

namespace Kites
{
KitesEditor::KitesEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    connect(this, &KitesEditor::blockCountChanged, this, &KitesEditor::updateViewPortMargins);
    connect(this, &KitesEditor::updateRequest, this, &KitesEditor::updateGutterColumns);
    connect(this, &KitesEditor::cursorPositionChanged, this, &KitesEditor::highlightCurrentLine);
    connect(&FontManager::getInstance(), &FontManager::fontChangedSignal, this, &QWidget::setFont);
    connect(&ThemeManager::getInstance(), &ThemeManager::editorThemeChangedSignal, this,
            &KitesEditor::themeChangedSlot);

    setFont(FontManager::getInstance().currentFont());
    applyEditorTheme();

    const int tapspace = 4;
    setTabStopDistance(tapspace * fontMetrics().horizontalAdvance(' '));
    updateViewPortMargins();
    highlightCurrentLine();
}

void KitesEditor::updateViewPortMargins()
{
    setViewportMargins(leftViewMargin(), 0, rightViewMargin(), 0);
}
void KitesEditor::updateGutterColumns(const QRect &rect, int dy)
{
    if(dy)
    {
        for (auto *column : m_rightGutterColumns)
        {
            column->scroll(0, dy);
        }
        for (auto *column : m_leftGutterColumns)
        {
            column->scroll(0, dy);
        }
    }
    else
    {
        for (auto *column : m_rightGutterColumns)
        {
            column->update(0, rect.y(), column->width(), rect.height());
        }
        for (auto *column : m_leftGutterColumns)
        {
            column->update(0, rect.y(), column->width(), rect.height());
        }
    }
}

void KitesEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly())
    {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = ThemeManager::getInstance().getCurrentLineColor();

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
    int currentWidth = 0;
    for (auto *column : m_leftGutterColumns)
    {
        const int width = column->sizeHint().width();
        column->setGeometry(QRect(cr.left() + currentWidth, cr.top(), width, cr.height()));
        currentWidth += width;
    }
    currentWidth = 0;
    for(auto* column : m_rightGutterColumns)
    {
        const int width = column->sizeHint().width();
        column->setGeometry(QRect(cr.right() - currentWidth - width, cr.top(), width, cr.height()));
        currentWidth += width;
    }
}

void KitesEditor::themeChangedSlot([[maybe_unused]] const QString &themeId)
{
    applyEditorTheme();
    highlightCurrentLine();
}

void KitesEditor::applyEditorTheme()
{
    // per-widget stylesheet so the editor can use a different theme than the
    // global UI (Zed-style editor/UI theme split); a per-widget palette is
    // ignored while an application stylesheet is active
    const ThemeManager &themeManager = ThemeManager::getInstance();
    setStyleSheet(QString("QPlainTextEdit { background-color: %1; color: %2; }")
                      .arg(themeManager.getEditorBackgroundColor().name(),
                           themeManager.getEditorForegroundColor().name()));

    // keep palette roles in sync for code that reads palette().color(QPalette::Base)
    QPalette editorPalette = palette();
    editorPalette.setColor(QPalette::Base, themeManager.getEditorBackgroundColor());
    editorPalette.setColor(QPalette::Text, themeManager.getEditorForegroundColor());
    setPalette(editorPalette);
    viewport()->update();
}

void KitesEditor::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange)
    {
        // Update the line number area when the palette changes
        highlightCurrentLine();
    }
    QPlainTextEdit::changeEvent(event);
}

void KitesEditor::addRightGutterColumn(GutterColumn* column)
{
    if (column && std::find(m_rightGutterColumns.begin(), 
    m_rightGutterColumns.end(), column) == m_rightGutterColumns.end())
    {
        m_rightGutterColumns.push_back(column);
    }
}

void KitesEditor::addLeftGutterColumn(GutterColumn* column)
{
    if (column && std::find(m_leftGutterColumns.begin(), 
    m_leftGutterColumns.end(), column) == m_leftGutterColumns.end())
    {
        m_leftGutterColumns.push_back(column);
    }
}

int KitesEditor::rightViewMargin() const
{
    int margin = 0;
    for (const auto *column : m_rightGutterColumns)
    {
        margin += column->sizeHint().width();
    }
    return margin;
}

int KitesEditor::leftViewMargin() const
{
    int margin = 0;
    for (const auto *column : m_leftGutterColumns)
    {
        margin += column->sizeHint().width();
    }
    return margin;
}
} // namespace Kites
