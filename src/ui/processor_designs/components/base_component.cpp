#include "ui/processor_designs/components/base_component.h"
#include <QPainter>
#include <QJsonObject>
namespace Kites
{
BaseComponent::BaseComponent(const QPainterPath &path, const QString &name, QGraphicsItem *parent)
    :QGraphicsPathItem(path,parent)
{
    setBrush(Qt::gray);
    setPen(QPen(Qt::white,2));

    //setFlag(QGraphicsItem::ItemIsSelectable, false);
    //setFlag(QGraphicsItem::ItemIsMovable, true);

    m_name  = name;
    if(name != "")
    {
        m_label = new QGraphicsTextItem(name, this);
        QRectF box = path.boundingRect();
        m_label->setPos(box.center().x() - m_label->boundingRect().width() / 2,
                        box.center().y() - m_label->boundingRect().height() / 2);
    }
}

void BaseComponent::setName(const QString& name)
{
    m_name = name;
    if(name != "")
    {
        m_label = new QGraphicsTextItem(name, this);
        QRectF box = path().boundingRect();
        m_label->setPos(box.center().x() - m_label->boundingRect().width() / 2,
                        box.center().y() - m_label->boundingRect().height() / 2);
    }

}

QJsonObject BaseComponent::toJson()
{
    QJsonObject obj;
    obj["type"] = "BaseComponent"; // childrens will override this
    obj["x"] = scenePos().x();
    obj["y"] = scenePos().y();
    obj["name"] = m_name;

    return obj;
}
}