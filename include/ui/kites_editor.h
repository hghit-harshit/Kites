#pragma once
#include <QPlainTextEdit>


namespace Kites
{
/**
 * @brief This will be the parent class for all code editors in Kites.
 * It is basically providing a line number area to the editors and 
 * 
 */
class KitesEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit KitesEditor(QWidget *parent = nullptr);
    ~KitesEditor(){};

protected:
class LineNumberArea : public QWidget
{
public:
    LineNumberArea(KitesEditor *editor) : QWidget(editor), m_editor(editor) {};
    QSize sizeHint() const override
    {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_editor->lineNumberAreaPaintEvent(event);
    }

private:
    KitesEditor *m_editor;
    
};
LineNumberArea *m_lineNumberArea = nullptr;
void updateLineNumberAreaWidth(int newBlockCount);
void updateLineNumberArea(const QRect &, int dy);
int lineNumberAreaWidth();
void lineNumberAreaPaintEvent(QPaintEvent *event);
void highlightCurrentLine();
void resizeEvent(QResizeEvent *event) override;
};
} // namespace Kites