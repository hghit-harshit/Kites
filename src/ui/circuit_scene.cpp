#include "ui/circuit_scene.h"
#include "ui/processor_designs/components/base_component.h"
#include "ui/processor_designs/components/alu_item.h"
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
#include <QTimer>
#include <QGraphicsProxyWidget>
namespace Kites
{
CircuitScene::CircuitScene(QObject *parent)
:QGraphicsScene(parent)
{
    setSceneRect(-2000, -2000, 4000, 4000);
    //setSceneRect(-1000, -1000, 2000, 2000);
    setBackgroundBrush(Qt::black);
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(100); // flash for 100ms

    connect(m_timer, &QTimer::timeout, this, [this]() 
    {
        if(m_stayActive)
            return;
        for (QGraphicsItem* item : items()) {
            WireItem* wire = dynamic_cast<WireItem*>(item);
            if (wire)
                wire->setActive(false);
        }
        update();
    });

    m_instructionTable = new QTableWidget();
    QGraphicsProxyWidget *proxy = addWidget(m_instructionTable);
    proxy->setPos(10, 10); // Position the table at (10,10) in the scene
    proxy->setZValue(1); 
}

void CircuitScene::updateCircuitState(const QList<QString>& wireList)
{
    // Iterate through all items in the scene
    // maybe i can make a different list of wires to optimize this

    // some of the wires will always be active
    

    for (QGraphicsItem* item : items())
    {
        // Check if the item is a WireItem
        WireItem* wire = dynamic_cast<WireItem*>(item);
        if (wire)
        {
            // Update the wire's state based on the wireList
            if (wireList.contains(wire->getName()))
            {
                wire->setActive(true); // Example: set active if in the list
            }
            else
            {
                wire->setActive(false); // Otherwise, set inactive
            }
        }
    }

    // Request a redraw of the scene to reflect changes
    update();
    m_timer->start(); // restart the timer to turn off the wires after interval
} 

void CircuitScene::vmStateChangedSlot(const QMap<QString, QVariant> &vmState)
{
    m_instructionTable->clear();
    QVariantMap instructionMap = vmState.value("CurrentInstructions").toMap();
    m_instructionTable->setColumnCount(instructionMap.size());
    m_instructionTable->setRowCount(1);
    
    int row = 0;
    for(auto it = instructionMap.begin(); it != instructionMap.end(); ++it)
    {
        int col = it.key().toInt();
        QTableWidgetItem* item = new QTableWidgetItem(it.value().toString());
        m_instructionTable->setItem(row, col, item);
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
            //wire->setPen(QPen(Qt::white, 2, Qt::SolidLine));

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

