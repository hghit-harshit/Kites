#ifndef RECT_ITEM_H
#define RECT_ITEM_H
#include "ui/processor_designs/components/base_component.h"
#include <QJsonObject>

namespace Kites
{
class RectItem : public BaseComponent
{
  public:
    RectItem(const QString &name = "", QGraphicsItem *parent = nullptr)
        : BaseComponent(
              []()
              {
                  QPainterPath path;
                  path.moveTo(0, 0);
                  path.lineTo(80, 0);
                  path.lineTo(80, 100);
                  path.lineTo(0, 100);
                  // aluPath.lineTo(0, 60);
                  // aluPath.lineTo(10, 50);
                  // aluPath.lineTo(0, 40);
                  // aluPath.lineTo(10, 100);
                  path.closeSubpath();
                  return path;
              }(),
              name, parent)
    {
    }

    QJsonObject toJson()
    {
        QJsonObject obj = BaseComponent::toJson();
        obj["type"] = "RectItem";

        return obj;
    }
};
} // namespace Kites
#endif // RECT_ITEM_H
