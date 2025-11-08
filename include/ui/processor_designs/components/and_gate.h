#ifndef AND_GATE_H
#define AND_GATE_H
#include "ui/processor_designs/components/base_component.h"
#include <QJsonObject>
namespace Kites
{
class AndGateItem : public BaseComponent
{
public:
    AndGateItem(const QString& name = "",QGraphicsItem *parent = nullptr)
        :BaseComponent
        ([](){
            QPainterPath path;
            qreal width = 20;   // total width (including arcs)
            qreal height = 30;   // total height (diameter of the semicircle)
            qreal radius = height / 2.0; // radius of semicircular ends

            // Start at left-center of capsule
            path.moveTo(0, -height/2);

            //left edge
            path.lineTo(width/2, -height/2);

            // right arc
            path.arcTo(QRectF(width/2, -height/2 , 2*radius, 2*radius), 90, -180);

            // right edge
            path.lineTo(0,height/2);

            // top arc
            //path.arcTo(QRectF(-width/2, -height/2, 2*radius, 2*radius), 0, 180);

            path.closeSubpath();
            return path;
        }(),
         "", parent){}

    QJsonObject toJson()
    {
        QJsonObject obj = BaseComponent::toJson();
        obj["type"] = "AndGateItem";

        return obj;
    }
};
}
#endif // AND_GATE_H
