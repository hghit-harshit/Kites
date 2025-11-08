#ifndef SHORT_RECT_H
#define SHORT_RECT_H
#include "ui/processor_designs/components/base_component.h" 
#include <QJsonObject>

namespace Kites
{
class ShortRect : public BaseComponent
{
public:
    ShortRect(const QString& name = "",QGraphicsItem *parent = nullptr)
        :BaseComponent
        ([](){
            QPainterPath path;
            path.moveTo(0, 0);
            path.lineTo(20, 0);
            path.lineTo(20, 50);
            path.lineTo(0, 50);
            // aluPath.lineTo(0, 60);
            // aluPath.lineTo(10, 50);
            // aluPath.lineTo(0, 40);
            //aluPath.lineTo(10, 100);
            path.closeSubpath();
            return path;
        }(),
         name, parent){}

    QJsonObject toJson()
    {
        QJsonObject obj = BaseComponent::toJson();
        obj["type"] = "ShortRect";

        return obj;
    }
};
} // namespace Kites
#endif // SHORT_RECT_H
