#include "ui/processor_designs/components/wire_item.h"
#include <QJsonArray>

namespace Kites
{
WireItem::WireItem(const QPainterPath &path,QGraphicsItem* parent )
    : QGraphicsPathItem(path,parent)
{
    //setFlag(QGraphicsItem::ItemIsMovable, true);
    m_activePen = QPen(Qt::yellow, 2, Qt::SolidLine);
    m_inactivePen = QPen(Qt::gray, 1, Qt::SolidLine);
    setPen(m_inactivePen);
}

QString WireItem::getName()
{
    return m_name;
}

void WireItem::setName(const QString& name)
{
    m_name = name;
}

void WireItem::setActive(bool isActive)
{
    if(isActive)
    {
        setPen(m_activePen);
    }
    else
    {
        setPen(m_inactivePen);
    }
}
QRectF WireItem::boundingRect() const
{
    //extending the bounding rect otherwise the arrow
    // leave ghost trails
    QRectF rect = QGraphicsPathItem::boundingRect();
    rect.adjust(-10, -10, 10, 10);
    return rect;
}

QPainterPath WireItem::shape() const {
    QPainterPathStroker stroker;
    stroker.setWidth(4);            // clickable "thickness"
    stroker.setJoinStyle(Qt::RoundJoin);
    stroker.setCapStyle(Qt::RoundCap);
    return stroker.createStroke(path());
}

void WireItem::addJunction(const QPointF &scenePos)
{
    if(!m_junctions.contains(scenePos))
    {
        m_junctions.append(scenePos);
    }
}

void WireItem::popJunction()
{
    m_junctions.pop_back();
}

void WireItem::saveArrowHead()
{
    m_arrowHeads.append(m_currentArrowHead);
}

void WireItem::addArrowHead(const QPolygonF &arrowHead)
{
    m_arrowHeads.append(arrowHead);
}
void WireItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    QGraphicsPathItem::paint(painter,option,widget);

    QPainterPath currentPath = this->path();

    if(currentPath.elementCount() < 2)
    { return;} // we have nothing to draw

    QPainterPath::Element last = currentPath.elementAt(currentPath.elementCount() -1);
    QPainterPath::Element secondLast = currentPath.elementAt(currentPath.elementCount() -2);

    // now we get the points from the elements
    QPointF p1(secondLast.x,secondLast.y);
    QPointF p2(last.x,last.y);

    // now make a line out of it and find its direction
    QLineF line(p1,p2);
    double angle = std::atan2(-line.dy(),line.dx());


    //now make the arrowhead
    const double arrowSize = 6;

    QPointF arrowP1 = p2 + QPointF(std::sin(angle - M_PI / 3) * arrowSize,
                                   std::cos(angle - M_PI / 3) * arrowSize);
    QPointF arrowP2 = p2 + QPointF(std::sin(angle - M_PI + M_PI / 3) * arrowSize,
                                   std::cos(angle - M_PI + M_PI / 3) * arrowSize);

    //QPolygonF arrowHead;
    m_currentArrowHead.clear();
    m_currentArrowHead << p2 << arrowP1 << arrowP2;
    painter->setRenderHint(QPainter::Antialiasing, true);
    //painter->setPen(pen());
    painter->drawPolygon( m_currentArrowHead);

    for (const QPointF &point : m_junctions)
    {
        // Draw an 8x8 circle (radius 4)
        painter->drawEllipse(point, 2, 2);
    }

    for(const QPolygonF &arrow : m_arrowHeads)
    {
        painter->drawPolygon(arrow);
    }
}

QJsonObject WireItem::toJson()
{
    QJsonObject obj;
    obj["type"] = "WireItem";

    // 1. Save the Path (same as before)
    QJsonArray pathArray;
    QPainterPath p = path();
    for(int i = 0; i < p.elementCount(); ++i)
    {
        QPainterPath::Element e = p.elementAt(i);
        QJsonObject pointObj;
        pointObj["x"] = e.x;
        pointObj["y"] = e.y;
        pointObj["type"] = (int)e.type; // wether its lineTo of moveTo
        pathArray.append(pointObj);
    }
    obj["path"] = pathArray;

    // --- THIS IS THE NEW PART ---
    // 2. Save the Junctions
    QJsonArray junctionsArray;
    for (const QPointF &point : m_junctions)
    {
        QJsonObject pointObj;
        pointObj["x"] = point.x();
        pointObj["y"] = point.y();
        junctionsArray.append(pointObj);
    }
    obj["junctions"] = junctionsArray;

    QJsonArray arrowHeadArray;
    for(const QPolygonF &arrowHead : m_arrowHeads)
    {
        QJsonArray polygonPointArray;

        // 4. Loop over the points in this single polygon
        for(const QPointF &point : arrowHead)
        {
            // 5. Create a JSON object for each point
            QJsonObject pointObj;
            pointObj["x"] = point.x();
            pointObj["y"] = point.y();
            polygonPointArray.append(pointObj);
        }
        arrowHeadArray.append(polygonPointArray);

    }
    obj["arrowHeads"] = arrowHeadArray;

    return obj;
}
} // namespace Kites
