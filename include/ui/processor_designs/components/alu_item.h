#ifndef ALU_H
#define ALU_H
#include "ui/processor_designs/components/base_component.h"
#include <QJsonObject>
namespace Kites
{
class ALUItem : public BaseComponent
{
  public:
    ALUItem(QGraphicsItem *parent = nullptr)
        : BaseComponent(
              []()
              {
                  QPainterPath aluPath;
                  aluPath.moveTo(0, 0);
                  aluPath.lineTo(40, 30);
                  aluPath.lineTo(40, 70);
                  aluPath.lineTo(0, 100);
                  aluPath.lineTo(0, 60);
                  aluPath.lineTo(10, 50);
                  aluPath.lineTo(0, 40);
                  // aluPath.lineTo(10, 100);
                  aluPath.closeSubpath();
                  return aluPath;
              }(),
              "", parent)
    {
    }

    QJsonObject toJson()
    {
        QJsonObject obj = BaseComponent::toJson();
        obj["type"] = "ALUItem";

        return obj;
    }
};
} // namespace Kites
#endif // ALU_H
