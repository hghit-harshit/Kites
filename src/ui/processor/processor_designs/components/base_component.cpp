#include "base_component.h"
#include <QJsonObject>
#include <QPainter>
#include <QTextDocument>
#include <QTextOption>
#include <algorithm>

namespace Kites
{
BaseComponent::BaseComponent(const QPainterPath &path, const QString &name, QGraphicsItem *parent)
    : QGraphicsPathItem(path, parent)
{
    setBrush(Qt::gray);
    setPen(QPen(Qt::white, 2));

    // setFlag(QGraphicsItem::ItemIsSelectable, false);
    // setFlag(QGraphicsItem::ItemIsMovable, true);

    m_name = name;
    if (name != "")
    {
        m_label = new QGraphicsTextItem(name, this);
        QFont labelFont = m_label->font();
        labelFont.setStretch(130);
        m_label->setFont(labelFont);
        QRectF box = path.boundingRect();
        const qreal labelWidth = std::max<qreal>(box.width() * 1.6, 80.0);
        m_label->setTextWidth(labelWidth);
        QTextOption option = m_label->document()->defaultTextOption();
        option.setAlignment(Qt::AlignHCenter);
        m_label->document()->setDefaultTextOption(option);
        m_label->setPos(box.center().x() - labelWidth / 2,
                        box.center().y() - m_label->boundingRect().height() / 2);
    }
}

void BaseComponent::setName(const QString &name)
{
    m_name = name;
    if (name != "")
    {
        m_label = new QGraphicsTextItem(name, this);
        QFont labelFont = m_label->font();
        labelFont.setStretch(130);
        m_label->setFont(labelFont);
        QRectF box = path().boundingRect();
        const qreal labelWidth = std::max<qreal>(box.width() * 1.6, 80.0);
        m_label->setTextWidth(labelWidth);
        QTextOption option = m_label->document()->defaultTextOption();
        option.setAlignment(Qt::AlignHCenter);
        m_label->document()->setDefaultTextOption(option);
        m_label->setPos(box.center().x() - labelWidth / 2,
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
} // namespace Kites