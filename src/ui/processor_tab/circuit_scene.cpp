#include "circuit_scene.h"
#include "processor_designs/components/alu_item.h"
#include "processor_designs/components/and_gate.h"
#include "processor_designs/components/base_component.h"
#include "processor_designs/components/hori_rect.h"
#include "processor_designs/components/mux_item.h"
#include "processor_designs/components/pipeline_reg.h"
#include "processor_designs/components/rect_item.h"
#include "processor_designs/components/short_alu.h"
#include "processor_designs/components/short_rect.h"
#include "processor_designs/components/wire_item.h"
#include <QFile>
#include <QGraphicsProxyWidget>
#include <QHeaderView>
#include <QIODevice>
#include <QInputDialog>
#include <QJsonArray>
#include <QLineEdit>
#include <QTimer>
#include <QJsonDocument>

namespace Kites
{

CircuitScene::CircuitScene(QObject *parent) : QGraphicsScene(parent)
{
    setSceneRect(-100, -400, 300, 500);
    // setSceneRect(-1000, -1000, 2000, 2000);
    setBackgroundBrush(Qt::black);
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(100); // flash for 100ms

    connect(m_timer, &QTimer::timeout, this,
            [this]()
            {
                if (m_stayActive)
                    return;
                for (QGraphicsItem *item : items())
                {
                    WireItem *wire = dynamic_cast<WireItem *>(item);
                    if (wire)
                        wire->setActive(false);
                }
                update();
            });

    setUpInstructionTable();
}

void CircuitScene::setUpInstructionTable()
{
    m_instructionTable = new QTableWidget();
    m_instructionTable->verticalHeader()->setVisible(false);
    m_instructionTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Add the table to the scene so fitTableToCircuit can locate and position it
    QGraphicsProxyWidget *proxy = addWidget(m_instructionTable);
    proxy->setZValue(1000); // keep table on top of circuit items
    proxy->setPos(0, 0);
    // fitTableToCircuit();
}

void CircuitScene::fitTableToCircuit(const QString &vmType)
{
    if (!m_instructionTable)
        return;

    // ── 1. Define columns based on vmType ──
    if (vmType == "pipeline")
    {
        m_columnKeys = {"IF/ID", "ID/EX", "EX/MEM", "MEM/WB"};
    }
    else if (vmType == "single_cycle")
    {
        m_columnKeys = {"CI"};
    }
    else
    {
        m_columnKeys = {"IF/ID", "ID/EX", "EX/MEM", "MEM/WB"}; // default
    }

    // ── 2. Set up table structure ONCE ──
    m_instructionTable->clear();
    m_instructionTable->setColumnCount(m_columnKeys.size());
    m_instructionTable->setRowCount(1);
    m_instructionTable->setHorizontalHeaderLabels(m_columnKeys);
    m_instructionTable->verticalHeader()->setVisible(false);
    m_instructionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_instructionTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_instructionTable->setFocusPolicy(Qt::NoFocus);

    // Bigger bold font for header
    QFont headerFont = m_instructionTable->horizontalHeader()->font();
    headerFont.setBold(true);
    headerFont.setPointSize(11);
    m_instructionTable->horizontalHeader()->setFont(headerFont);

    // Bigger bold font for cell values
    QFont cellFont = m_instructionTable->font();
    cellFont.setBold(true);
    cellFont.setPointSize(11);
    m_instructionTable->setFont(cellFont);

    // ── 3. Fit width to circuit bounds ──
    QRectF bounds;
    for (QGraphicsItem *item : items())
    {
        if (dynamic_cast<QGraphicsProxyWidget *>(item))
            continue;
        bounds = bounds.united(item->mapToScene(item->boundingRect()).boundingRect());
    }

    if (bounds.isEmpty() || bounds.width() == 0)
    {
        qWarning() << "fitTableToCircuit: empty bounds";
        return;
    }

    double colWidth = bounds.width() / m_columnKeys.size();
    for (int i = 0; i < m_columnKeys.size(); ++i)
        m_instructionTable->setColumnWidth(i, (int)colWidth);

    int totalWidth = (int)(colWidth * m_columnKeys.size());
    m_instructionTable->setFixedWidth(totalWidth + 4);
    m_instructionTable->setFixedHeight(m_instructionTable->horizontalHeader()->height() +
                                       m_instructionTable->rowHeight(0));

    // ── 4. Position above circuit ──
    for (QGraphicsItem *item : items())
    {
        QGraphicsProxyWidget *proxy = dynamic_cast<QGraphicsProxyWidget *>(item);
        if (proxy && proxy->widget() == m_instructionTable)
        {
            proxy->setPos(bounds.left(), bounds.top() - proxy->boundingRect().height() - 10);
            break;
        }
    }

    qDebug() << "Table set up with" << m_columnKeys.size() << "columns, width:" << totalWidth;
}


void CircuitScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    if (mouseEvent->button() == Qt::LeftButton)
    {
        // Right-click: Show context menu
        qInfo() << "Mouse pressed at:" << mouseEvent->scenePos();
    }
    else
    {
        // For other mouse buttons, call the base implementation
        QGraphicsScene::mousePressEvent(mouseEvent);
    }
}

void CircuitScene::updateCircuitState(const QList<QString> &wireList)
{
    // Iterate through all items in the scene
    // maybe i can make a different list of wires to optimize this

    // some of the wires will always be active

    for (QGraphicsItem *item : items())
    {
        // Check if the item is a WireItem
        WireItem *wire = dynamic_cast<WireItem *>(item);
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

void CircuitScene::loadScene(const QString &fileName)
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

    // clear() deletes all items, but we also need to reset the instruction table

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
            else if (type == "RectItem")
            {
                component = new RectItem();
            }
            else if (type == "HoriRect")
            {
                component = new HoriRect();
            }
            else if (type == "MuxItem")
            {
                component = new MuxItem();
            }
            else if (type == "PipelineReg")
            {
                component = new PipelineReg();
            }
            else if (type == "SALUItem")
            {
                component = new SALUItem();
            }
            else if (type == "ShortRect")
            {
                component = new ShortRect();
            }
            else if (type == "AndGateItem")
            {
                component = new AndGateItem();
            }
            else
            {
                // Unknown type
                qWarning() << "Unknown component type dropped:" << type;
                // event->ignore();
            }

            if (component)
            {
                // Read position
                double x = compObj["x"].toDouble();
                double y = compObj["y"].toDouble();
                component->setPos(x, y);

                // Read name
                if (compObj.contains("name"))
                {
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
            // wire->setPen(QPen(Qt::white, 2, Qt::SolidLine));

            // Set the name
            if (wireObj.contains("name"))
            {
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
                    // junctionPoints.append(QPointF(p["x"].toDouble(), p["y"].toDouble()));
                    wire->addJunction(QPointF(p["x"].toDouble(), p["y"].toDouble()));
                }
                // wire->setJunctions(junctionPoints);
            }

            if (wireObj.contains("arrowHeads") && wireObj["arrowHeads"].isArray())
            {
                // QList<QPolygonF> allPolygons;
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
                    // allPolygons.append(polygon);
                }
            }
            addItem(wire);
        }
    }

    qDebug() << "Scene loaded from" << fileName;
    setUpInstructionTable();

    // this is very very hacky but it works fow now
    //  i am sooo sorrry
    if (fileName.contains("single_cycle"))
    {
        fitTableToCircuit("single_cycle");
    }
    else
    {
        fitTableToCircuit("pipeline");
    }
}
} // namespace Kites
