#pragma once

#include "processor_designs/components/base_component.h"
#include "processor_designs/components/wire_item.h"
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainterPath>
#include <QPointF>
#include <QTableWidget>
#include <filesystem>


namespace Kites
{
class CircuitScene : public QGraphicsScene
{
    Q_OBJECT
  public:
    explicit CircuitScene(QObject *parent = nullptr);

    void handleMouseMove(const QPointF &scenePos);

    void loadScene(const QString &fileName);
    void setWireStayActive(bool stayActive)
    {
        m_stayActive = stayActive;
    }
    bool getWireStayActive() const
    {
        return m_stayActive;
    }

  protected:
    // void mouseMoveEvent(QGraphicsSceneMouseEvent *mouseEvent) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) override;

  private:
    std::filesystem::path m_sceneFilePath; // unused variable

    QTimer *m_timer;                            // well use it to make the wire blink
    QTableWidget *m_instructionTable = nullptr; // the table that shows the instructions
    bool m_stayActive = false;                  // whether to keep the wires active or not
    QStringList m_columnKeys;

    void setUpInstructionTable();
    void fitTableToCircuit(const QString &vmType);
  public slots:
    void updateCircuitState(const QList<QString> &wireList);
    // maybe later we merge this two functions but for now this will have to do
};
} // namespace Kites
