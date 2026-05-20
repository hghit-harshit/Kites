#include "ui/circuit_view.h"

namespace Kites
{
// void CircuitView::mouseMoveEvent(QMouseEvent *event)
// {
//     // Pass the mouse position to the scene
//     if (auto* circuitScene = dynamic_cast<CircuitScene*>(scene()))
//     {
//         circuitScene->handleMouseMove(mapToScene(event->position().toPoint()));
//     }

//     QGraphicsView::mouseMoveEvent(event); // Call base class
// }

CircuitView::CircuitView(QWidget *parent) : QGraphicsView(parent)
{
    // setAcceptDrops(true);
    setDragMode(QGraphicsView::NoDrag);
    setRenderHint(QPainter::Antialiasing, true);
    setMouseTracking(true);
    // fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}

CircuitView::CircuitView(QGraphicsScene *scene, QWidget *parent) : QGraphicsView(scene, parent)
{
    // Crucial: Tell the widget to accept drop events
    // setAcceptDrops(true);
    setDragMode(QGraphicsView::NoDrag);
    setMouseTracking(true);
    setRenderHint(QPainter::Antialiasing, true);
    centerOn(0, 0);

    // the table view that will show the instructions
    // Ensure the table is on top of other items

    // setBackgroundBrush(Qt::black);
    // fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

void CircuitView::showEvent(QShowEvent *event)
{
    QGraphicsView::showEvent(event);
    setTransform(QTransform().scale(0.7, 0.7));
    // fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}

/*
void CircuitView::drawBackground(QPainter *painter, const QRectF &rect)
{
    // 1. First, call the base class method to ensure any
    //    default behavior is preserved (like clearing)
    //    Alternatively, just fill it with white:
    painter->fillRect(rect, Qt::black);

    // 2. Define your grid size and dot size
    int gridSize = 20; // 20 pixels apart
    qreal dotRadius = 1.0; // 1-pixel radius for each dot

    // 3. Set the color for the dots
    QBrush dotBrush(QColor(200, 200, 200)); // Light gray
    painter->setBrush(dotBrush);
    painter->setPen(Qt::NoPen); // We don't want an outline on the dots

    // 4. Find the top-left-most dot to start drawing
    //    This optimizes drawing to only be in the visible 'rect'
    qreal left = int(rect.left()) - (int(rect.left()) % gridSize);
    qreal top = int(rect.top()) - (int(rect.top()) % gridSize);

    // 5. Loop over the visible 'rect' and draw dots
    for (qreal x = left; x < rect.right(); x += gridSize)
    {
        for (qreal y = top; y < rect.bottom(); y += gridSize)
        {
            // Draw a small ellipse (a circle) at the grid point
            painter->drawEllipse(QPointF(x, y), dotRadius, dotRadius);
        }
    }
} */
} // namespace Kites