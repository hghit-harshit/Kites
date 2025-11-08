#ifndef PIPELINE_REG_H
#define PIPELINE_REG_H
#include "ui/processor_designs/components/base_component.h"
#include <QJsonObject>

namespace Kites
{
class PipelineReg : public BaseComponent
{
public:
    PipelineReg(const QString& name = "",QGraphicsItem *parent = nullptr)
        :BaseComponent
        ([](){
            QPainterPath path;
            path.moveTo(0, 0);
            path.lineTo(20, 0);
            path.lineTo(20, 450);
            path.lineTo(0, 450);
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
        obj["type"] = "PipelineReg";

        return obj;
    }
};
} // namespace Kites
#endif // PIPELINE_REG_H
