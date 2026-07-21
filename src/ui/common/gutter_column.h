#pragma once
#include <QWidget>
#include <QTextBlock>
#include "kites_editor.h"
namespace Kites
{
class GutterColumn : public QWidget
{
    Q_OBJECT
public:
    explicit GutterColumn(KitesEditor* editor = nullptr)
        : QWidget(editor), m_editor(editor) {};
    virtual QSize sizeHint() const = 0;
protected:
    virtual void  paintEvent(QPaintEvent *event) = 0;
    QTextBlock firstVisibleBlock() const
    {
        return m_editor->firstVisibleBlock();
    }
    int blockTop(const QTextBlock &block) const
    {
        return static_cast<int>(
            m_editor->blockBoundingGeometry(block).translated(m_editor->contentOffset()).top());
    }
    int blockHeight(const QTextBlock &block) const
    {
        return static_cast<int>(m_editor->blockBoundingRect(block).height());
    }
    KitesEditor* m_editor;
};
}// namespace Kites
