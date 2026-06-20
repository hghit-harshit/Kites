#ifndef MUX_ITEM_H
#define MUX_ITEM_H
#include "base_component.h"
#include <QJsonObject>
namespace Kites
{
class MuxItem : public BaseComponent
{
  public:
    MuxItem(const QString &name = "", QGraphicsItem *parent = nullptr)
        : BaseComponent(
              []()
              {
                  QPainterPath path;
                  qreal width = 20;           // total width (including arcs)
                  qreal height = 60;          // total height (diameter of the semicircle)
                  qreal radius = width / 2.0; // radius of semicircular ends

                  // Start at left-center of capsule
                  path.moveTo(-width / 2, -height / 2 + radius);

                  // left edge
                  path.lineTo(-width / 2, height / 2 - radius);

                  // bottom arc
                  path.arcTo(QRectF(-width / 2, height / 2 - 2 * radius, 2 * radius, 2 * radius),
                             180, 180);

                  // right edge
                  path.lineTo(width / 2, -height / 2 + radius);

                  // top arc
                  path.arcTo(QRectF(-width / 2, -height / 2, 2 * radius, 2 * radius), 0, 180);

                  path.closeSubpath();
                  return path;
              }(),
              "", parent)
    {
        Q_UNUSED(name);
    }

    QJsonObject toJson()
    {
        QJsonObject obj = BaseComponent::toJson();
        obj["type"] = "MuxItem";

        return obj;
    }
};
} // namespace Kites
#endif // MUX_ITEM_H
