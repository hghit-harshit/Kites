#ifndef HORI_RECT_H
#define HORI_RECT_H

#include "base_component.h"
#include <QJsonObject>

namespace Kites
{
class HoriRect : public BaseComponent
{
  public:
    HoriRect(const QString &name = "", QGraphicsItem *parent = nullptr)
        : BaseComponent(
              []()
              {
                  QPainterPath path;
                  path.moveTo(0, 0);
                  path.lineTo(80, 0);
                  path.lineTo(80, 40);
                  path.lineTo(0, 40);
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
        obj["type"] = "HoriRect";

        return obj;
    }
};
} // namespace Kites

#endif // HORI_RECT_H
