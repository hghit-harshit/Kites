#pragma once
#include <QGraphicsView>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>

namespace Kites
{

class CircuitView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit CircuitView(QWidget* parent = nullptr);
    explicit CircuitView(QGraphicsScene *scence, QWidget *parent = nullptr);
protected:
    //void dragEnterEvent(QDragEnterEvent* event) override;
    //void dragMoveEvent(QDragMoveEvent *event)override;
    //void dropEvent(QDropEvent *event)override;
    //void mouseMoveEvent(QMouseEvent *mouseEvent) override;
    //void drawBackground(QPainter *painter, const QRectF &rect) override;
    void showEvent(QShowEvent *event) override;
};

} // namespace Kites
