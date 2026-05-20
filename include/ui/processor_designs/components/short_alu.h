#ifndef SHORT_ALU_H
#define SHORT_ALU_H
#include "ui/processor_designs/components/base_component.h"
#include <QJsonObject>
namespace Kites
{
class SALUItem : public BaseComponent
{
  public:
    SALUItem(QGraphicsItem *parent = nullptr)
        : BaseComponent(
              []()
              {
                  QPainterPath aluPath;
                  aluPath.moveTo(0, 0);
                  aluPath.lineTo(20, 15);
                  aluPath.lineTo(20, 35);
                  aluPath.lineTo(0, 50);
                  aluPath.lineTo(0, 30);
                  aluPath.lineTo(5, 25);
                  aluPath.lineTo(0, 20);
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
        obj["type"] = "SALUItem";

        return obj;
    }
};
} // namespace Kites
#endif // SHORT_ALU_H
