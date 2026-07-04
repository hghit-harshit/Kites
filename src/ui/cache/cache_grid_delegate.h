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
        // QColor stableLineColor = (option.palette.window().color().value() < 128) 
        //                  ? QColor(255, 255, 255)  // If background is dark, force white
        //                  : QColor(0, 0, 0);
        int borderType = index.model()->data(index, Qt::UserRole + 1).toInt();
        if (borderType == 1)
        {
            QPen pen(QColor(255, 255, 255), 1);
            painter->setPen(pen);
        }
        else
        {
            QPen pen(QColor(255, 255, 255), 1);
            pen.setStyle(Qt::DotLine);
            painter->setPen(pen);   
        }
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
        painter->restore();
    }

};
} // namespace Kites