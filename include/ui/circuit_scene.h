#pragma once

#include <QGraphicsScene>
#include <QPointF>
#include <QGraphicsSceneMouseEvent>
#include <QPainterPath>
#include "ui/processor_designs/components/wire_item.h"
#include "ui/processor_designs/components/base_component.h"

namespace Kites
{
class CircuitScene : public QGraphicsScene
{
    Q_OBJECT
public:
    explicit CircuitScene(QObject *parent = nullptr);

    void startDrawingWire(const QPointF&);
    void handleMouseMove(const QPointF& scenePos);
    bool tryStartWireAt(const QPointF& scenePos);

    void saveScene(const QString& fileName);
    void loadScene(const QString& fileName);
protected:
    //void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) override;

private:
    QPointF getSnappedPos(BaseComponent* component,const QPointF& scenePos);
    BaseComponent* componentAt(const QPointF& scenePos);
    WireItem* wireAt(const QPointF& scenePos);
    bool m_isDrawingWire;
    WireItem* m_tempWire;
    QPainterPath m_tempPainterPath;

    QPainterPath m_originalPath; // when we add junction of a part this will store the original path
    bool m_isBranch; // indicates that the current wire is branch of an existing one

};
} // namespace Kites
