#pragma once
#include <QGraphicsPathItem>
#include <QPainterPath>

namespace Kites
{


class BaseComponent : public QGraphicsPathItem
{
    public:
        BaseComponent(const QPainterPath &path, const QString &name, QGraphicsItem *parent = nullptr);
    
    private:
        QGraphicsTextItem *m_label;
    
};
} // namespace Kites