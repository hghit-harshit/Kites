#include "ui/circuit_scene.h"
#include "ui/processor_designs/components/base_component.h"
#include "ui/processor_designs/components/alu.h"
#include "ui/processor_designs/components/wire_item.h"
#include "ui/processor_designs/components/rect_item.h"
#include "ui/processor_designs/components/mux_item.h"
#include "ui/processor_designs/components/pipeline_reg.h"
#include "ui/processor_designs/components/short_alu.h"
#include "ui/processor_designs/components/hori_rect.h"
#include "ui/processor_designs/components/short_rect.h"
#include "ui/processor_designs/components/and_gate.h"
#include <QLineEdit>
#include <QInputDialog>
#include <QJsonArray>
#include <QFile>
#include <QIODevice>

namespace Kites
{
CircuitScene::CircuitScene(QObject *parent)
    :m_isDrawingWire(false),
    m_tempWire(nullptr),
    m_isBranch(false)
{
    setSceneRect(-2000, -2000, 4000, 4000);
}

void CircuitScene::startDrawingWire(const QPointF& startPos)
{
    qDebug() << "Is this even called?";
    m_isDrawingWire = true;
    m_tempPainterPath = QPainterPath(startPos);
    m_tempWire = new WireItem(m_tempPainterPath);

    m_tempWire->setPath(m_tempPainterPath);

    QPen pen(Qt::white, 2);
    pen.setStyle(Qt::DashLine);
    m_tempWire->setPen(pen);

    addItem(m_tempWire);
}

// void CircuitScene::mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent)
// {
//     if(m_isDrawingWire)
//     {
//         qDebug() << "mouseMoveEvent is being called.";
//         QPointF mousePos = mouseEvent->scenePos();

//         QPainterPath livePath = m_tempPainterPath;
//         livePath.lineTo(mousePos);

//         m_tempWire->setPath(livePath);
//     }
//     else
//     {
//         QGraphicsScene::mouseMoveEvent(mouseEvent);
//     }
// }

// returns the component we have right cliked on
BaseComponent* CircuitScene::componentAt(const QPointF& scenePos)
{
    QList<QGraphicsItem*> itemsList = items(scenePos);

    // Loop through all the items
    for (QGraphicsItem *item : itemsList)
    {
        // If the item is the wire we're currently drawing, skip it
        if (item == m_tempWire)
        {
            continue;
        }

        auto* component = dynamic_cast<BaseComponent*>(item);
        if (component)
        {
            return component; // Clicked directly on the component
        }

        // If that fails, check if we clicked a child of the component
        if (item->parentItem())
        {
            component = dynamic_cast<BaseComponent*>(item->parentItem());
            if (component)
            {
                return component;
            }
        }
    }

    return nullptr;
}


QPointF CircuitScene::getSnappedPos(BaseComponent* component,const QPointF& scenePos)
{
    QRectF localRect = component->boundingRect();

    QPointF localClickPos = component->mapFromScene(scenePos);

    //calculate distances from the edges
    qreal distLeft = qAbs(localClickPos.x() - localRect.left());
    qreal distRight = qAbs(localClickPos.x() - localRect.right());
    qreal distTop = qAbs(localClickPos.y() - localRect.top());
    qreal distBottom = qAbs(localClickPos.y() - localRect.bottom());

    // 4. Find the minimum distance
    qreal minDistance = std::min({distLeft, distRight, distTop, distBottom});

    // 5. Create the new snapped position
    //    Start it at the click position
    QPointF snappedLocalPos = localClickPos;

    // 6. Snap the closest coordinate to its edge
    if (minDistance == distLeft)
    {
        snappedLocalPos.setX(localRect.left());
    }
    else if (minDistance == distRight)
    {
        snappedLocalPos.setX(localRect.right());
    }
    else if (minDistance == distTop)
    {
        snappedLocalPos.setY(localRect.top());
    }
    else // (minDistance == distBottom)
    {
        snappedLocalPos.setY(localRect.bottom());
    }

    // bounding the position to be insidde component bound so it does not start from out side
    // in case we drop wore at a corner
    snappedLocalPos.setX( qBound(localRect.left(), snappedLocalPos.x(), localRect.right()) );
    snappedLocalPos.setY( qBound(localRect.top(), snappedLocalPos.y(), localRect.bottom()) );

    //map the coords back to scene coords
    return component->mapToScene(snappedLocalPos);
}

bool CircuitScene::tryStartWireAt(const QPointF &scenePos)
{
    BaseComponent* startComponent = componentAt(scenePos);

    if(startComponent)
    {
        startDrawingWire(getSnappedPos(startComponent,scenePos));
        return true;
    }
    else{ return false; }
}

WireItem* CircuitScene::wireAt(const QPointF& scenePos)
{
    QGraphicsItem* item = itemAt(scenePos,QTransform());

    return dynamic_cast<WireItem*>(item);
}

void CircuitScene::handleMouseMove(const QPointF& scenePos)
{
    if(m_isDrawingWire)
    {
        QPainterPath livePath = m_tempPainterPath;
        livePath.lineTo(scenePos);
        //m_tepmWire->prepareGeometryChange();
        m_tempWire->setPath(livePath);
        //m_tempWire->update();
    }
}

void CircuitScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if(m_isDrawingWire)
    {
        // QGraphicsItem *itemAtPos = itemAt(mouseEvent->scenePos(), QTransform());
        // DatapathComponent *component = dynamic_cast<DatapathComponent*>(itemAtPos);

        //for now lets just see
        if(mouseEvent->button() == Qt::LeftButton)
        {
            m_tempPainterPath.lineTo(mouseEvent->scenePos());
            m_tempWire->setPath(m_tempPainterPath);
        }
        else if(mouseEvent->button() == Qt::RightButton)
        {
            m_isDrawingWire = false;

            BaseComponent* endComponent = componentAt(mouseEvent->scenePos());
            if (endComponent) // Valid finish
            {
                m_tempPainterPath.lineTo(getSnappedPos(endComponent,mouseEvent->scenePos()));
                m_tempWire->setPath(m_tempPainterPath);
                m_tempWire->setPen(QPen(Qt::white, 2, Qt::SolidLine));

                if(!m_isBranch) // if its not a branch we prompt for name
                {
                    bool ok;
                    // We use viewport() as the parent for the dialog
                    QString name = QInputDialog::getText(nullptr, "Name Component",
                                                         "Component name:", QLineEdit::Normal,
                                                         "component_name", &ok);
                    m_tempWire->setName(name);
                    m_tempWire = nullptr;
                }
                m_isBranch = false; // if it was a branch it ended so set it false again
            }
            else // Invalid finish (on background)
            {
                if(!m_isBranch) // if this is not a branch delete the whole wire
                {
                    removeItem(m_tempWire);
                    delete m_tempWire;
                    m_tempWire = nullptr;
                }
                else
                {
                    m_tempWire->setPath(m_originalPath);
                    m_tempWire->popJunction();
                    m_isBranch = false;
                    m_tempPainterPath = QPainterPath();
                    m_tempWire = nullptr;
                }
            }
        }
    }
    else if(!m_isDrawingWire && mouseEvent->button() == Qt::LeftButton)
    {
        WireItem* clickedWire = wireAt(mouseEvent->scenePos());
        if(!clickedWire) // if we left clicked but not on a wire we pass the event above
        {
            QGraphicsScene::mousePressEvent(mouseEvent);
            return;
        }
        m_isDrawingWire = true;
        m_isBranch = true;
        m_tempWire = clickedWire; // Modifying an existing wire
        m_originalPath = m_tempWire->path(); // save the original wire
        m_tempPainterPath = m_originalPath;
        m_tempPainterPath.moveTo(mouseEvent->scenePos());
        m_tempWire->addJunction(mouseEvent->scenePos()); // add this junction to wire
        m_tempWire->saveArrowHead(); //save previous arrow head
    }
    else if(mouseEvent->button() == Qt::RightButton)
    {
        // if we right click on top of a component or wire we delete that
        QGraphicsItem *item = itemAt(mouseEvent->scenePos(),QTransform());
        removeItem(item);
    }
    else
    {
        QGraphicsScene::mousePressEvent(mouseEvent);
    }
}

void CircuitScene::saveScene(const QString& fileName)
{
    qDebug() << "Scene has" << items().size() << "items.";
    qDebug() << "save scene is being called.";
    qDebug() << "Scene has" << items().size() << "items.";
    qDebug() << "Scene has" << items().size() << "items.";
    QJsonObject sceneJson;
    QJsonArray componentsArray;
    QJsonArray wiresArray;

    // Loop through all items in the scene

    for (QGraphicsItem *item : items())
    {
        if(!item)continue;
        // Try to cast to your types
        if (auto* component = dynamic_cast<BaseComponent*>(item))
        {
            componentsArray.append(component->toJson());
        }
        else if (auto* wire = dynamic_cast<WireItem*>(item))
        {
            qDebug() << "Do we even enter save wire.";
            // Only save permanent wires, not the one being drawn
            // if (m_tempWire && wire != m_tempWire)
            // {
                wiresArray.append(wire->toJson());
            //}
        }
    }

    // Add the arrays to the main JSON object
    sceneJson["components"] = componentsArray;
    sceneJson["wires"] = wiresArray;

    // Write the JSON to a file
    QJsonDocument doc(sceneJson);
    QFile file(fileName);

    if (file.open(QIODevice::WriteOnly))
    {
        file.write(doc.toJson());
        file.close();
        qDebug() << "Scene saved to" << fileName;
    }
    else
    {
        qWarning() << "Could not save file:" << file.errorString();
    }
}

void CircuitScene::loadScene(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Could not open file:" << file.errorString();
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(fileData);
    if (doc.isNull())
    {
        qWarning() << "Failed to parse JSON file.";
        return;
    }

    QJsonObject sceneJson = doc.object();

    // 2. Clear the current scene
    clear(); // Deletes all items from the scene

    // 3. Load Components (The "Factory")
    if (sceneJson.contains("components") && sceneJson["components"].isArray())
    {
        QJsonArray componentsArray = sceneJson["components"].toArray();
        for (const QJsonValue &compValue : componentsArray)
        {
            QJsonObject compObj = compValue.toObject();
            QString type = compObj["type"].toString();
            QString name = compObj["name"].toString();

            BaseComponent *component = nullptr;

            // --- Your Factory ---
            if (type == "ALUItem")
            {
                component = new ALUItem();
            }
            else if(type == "RectItem")
            {
                component = new RectItem();
            }
            else if(type == "HoriRect")
            {
                component = new HoriRect();
            }
            else if(type == "MuxItem")
            {
                component = new MuxItem();
            }
            else if(type == "PipelineReg")
            {
                component = new PipelineReg();

            }
            else if (type == "SALUItem")
            {
                component = new SALUItem();

            }
            else if(type == "ShortRect")
            {
                component = new ShortRect();

            }
            else if(type == "AndGateItem")
            {
                component = new AndGateItem();
            }
            else
            {
                // Unknown type
                qWarning() << "Unknown component type dropped:" << type;
                //event->ignore();
            }

            if (component)
            {
                // Read position
                double x = compObj["x"].toDouble();
                double y = compObj["y"].toDouble();
                component->setPos(x, y);

                // Read name
                if (compObj.contains("name")) {
                    component->setName(compObj["name"].toString());
                }

                addItem(component);
            }
        }
    }

    // 4. Load Wires
    if (sceneJson.contains("wires") && sceneJson["wires"].isArray())
    {
        QJsonArray wiresArray = sceneJson["wires"].toArray();
        for (const QJsonValue &wireValue : wiresArray)
        {
            QJsonObject wireObj = wireValue.toObject();

            // Re-create the path
            QJsonArray pathArray = wireObj["path"].toArray();
            QPainterPath path;
            for (int i = 0; i < pathArray.size(); ++i)
            {
                QJsonObject pointObj = pathArray[i].toObject();
                double x = pointObj["x"].toDouble();
                double y = pointObj["y"].toDouble();

                int type = pointObj["type"].toInt();

                if (type == QPainterPath::MoveToElement)
                {
                    path.moveTo(x, y); // This starts a new branch
                }
                else if (type == QPainterPath::LineToElement)
                {
                    path.lineTo(x, y); // This continues a branch
                }
            }

            // Create the wire
            WireItem *wire = new WireItem(path);
            wire->setPen(QPen(Qt::white, 2, Qt::SolidLine));

            // Set the name
            if (wireObj.contains("name")) {
                wire->setName(wireObj["name"].toString());
            }

            // Re-create the junction dots
            if (wireObj.contains("junctions") && wireObj["junctions"].isArray())
            {
                QJsonArray junctionsArray = wireObj["junctions"].toArray();
                QList<QPointF> junctionPoints;
                for (const QJsonValue &jval : junctionsArray)
                {
                    QJsonObject p = jval.toObject();
                    //junctionPoints.append(QPointF(p["x"].toDouble(), p["y"].toDouble()));
                    wire->addJunction(QPointF(p["x"].toDouble(), p["y"].toDouble()));
                }
                //wire->setJunctions(junctionPoints);
            }

            if (wireObj.contains("arrowHeads") && wireObj["arrowHeads"].isArray())
            {
                //QList<QPolygonF> allPolygons;
                QJsonArray arrowHeadArray = wireObj["arrowHeads"].toArray();
                for (const QJsonValue &polyVal : arrowHeadArray)
                {
                    QPolygonF polygon; // Create a new QPolygonF
                    QJsonArray polygonPointArray = polyVal.toArray();
                    for (const QJsonValue &pointVal : polygonPointArray)
                    {
                        QJsonObject p = pointVal.toObject();
                        polygon.append(QPointF(p["x"].toDouble(), p["y"].toDouble()));

                    }
                    wire->addArrowHead(polygon);
                    //allPolygons.append(polygon);
                }
            }
            addItem(wire);
        }
    }

    qDebug() << "Scene loaded from" << fileName;
}
} // namespace Kites