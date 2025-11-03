#include "ui/processor_designs/components/base_component.h"
#include <QPainter>

namespace Kites
{
    BaseComponent::BaseComponent(const QPainterPath &path, const QString &name, QGraphicsItem *parent)
    :QGraphicsPathItem(path,parent)
    {
        setBrush(Qt::gray);
        setPen(QPen(Qt::black,2));

        setFlag(QGraphicsItem::ItemIsSelectable, false);
        setFlag(QGraphicsItem::ItemIsMovable, true);

        m_label = new QGraphicsTextItem(name, this);
        QRectF box = path.boundingRect();
        m_label->setPos(box.center().x() - m_label->boundingRect().width() / 2,
                        box.center().y() - m_label->boundingRect().height() / 2);
    }
}