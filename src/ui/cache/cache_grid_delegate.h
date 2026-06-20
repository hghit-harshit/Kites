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
    explicit CacheGridDelegate(size_t num_ways, QObject *parent = nullptr)
        : QStyledItemDelegate(parent), num_ways_(num_ways)
    {
    }

    void UpdateNumWays(size_t num_ways)
    {
        num_ways_ = num_ways;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        // draw the cell normally first
        QStyledItemDelegate::paint(painter, option, index);

        painter->save();

        size_t way = static_cast<size_t>(index.row()) % num_ways_;

        // ── hard line at the top of each new set (way == 0) ──────────────────
        if (way == 0 && index.row() != 0)
        {
            QPen hard_pen(option.palette.color(QPalette::Text), 1.5);
            painter->setPen(hard_pen);
            painter->drawLine(option.rect.topLeft(), option.rect.topRight());
        }
        // ── faint line between ways within the same set ───────────────────────
        else if (way != 0)
        {
            QPen faint_pen(option.palette.color(QPalette::Mid), 0.5);
            faint_pen.setStyle(Qt::DotLine);
            painter->setPen(faint_pen);
            painter->drawLine(option.rect.topLeft(), option.rect.topRight());
        }

        painter->restore();
    }

  private:
    size_t num_ways_;
};

} // namespace Kites