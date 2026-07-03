// ui/cache_grid_delegate.h
#pragma once
#include <QPainter>
#include <QStyledItemDelegate>


namespace Kites
{

class CacheGridDelegate : public QStyledItemDelegate
{
    Q_OBJECT
  public:
    explicit CacheGridDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {}


    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        // draw the cell normally first
        QStyledItemDelegate::paint(painter, option, index);

        painter->save();
        int borderType = index.model()->data(index, Qt::UserRole + 1).toInt();
        if (borderType == 1)
        {
            QPen pen(option.palette.color(QPalette::Text), 1.5);
            painter->setPen(pen);
        }
        else
        {
            QPen pen(option.palette.color(QPalette::Mid), 0.5);
            pen.setStyle(Qt::DotLine);
            painter->setPen(pen);
        }
        painter->drawLine(option.rect.topLeft(), option.rect.topRight());
        painter->restore();
    }

};
} // namespace Kites