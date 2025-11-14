#pragma once

#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QPainter>
#include <QPolygonF>
#include <QJsonObject>
#include <QPen>
namespace Kites
{

class WireItem : public QGraphicsPathItem
{
public:
    explicit WireItem(const QPainterPath &path,QGraphicsItem* parent = nullptr);
    void setName(const QString& name);
    QString getName();
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    QPainterPath shape() const override;
    //void updatePath(const QPainterPath& newPath);
    QRectF boundingRect() const override;
    void addJunction(const QPointF& scenePos);
    void popJunction();
    void addArrowHead(const QPolygonF& arroHead);
    void saveArrowHead();
    void setActive(bool isActive);
    QJsonObject  toJson();
private:
    QString m_name;
    QList<QPointF> m_junctions; // these are all the point where we branch the wire
    QList<QPolygonF> m_arrowHeads;
    QPolygonF m_currentArrowHead;
    
    QPen m_activePen;
    QPen m_inactivePen;
};
} // namespace Kites