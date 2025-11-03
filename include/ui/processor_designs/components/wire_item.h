#pragma once

#include <QGraphicsPathItem>
#include <QPen>
class WireItem : public QGraphicsPathItem
{
    public:
        WireItem(QGraphicsItem* parent);
        void setName(const QString& name);
        QString getName();

        void setActive(bool active);

    private:
        QString m_name;
        bool m_isActive;
        QPen m_activePen;
        QPen m_inactivePen;
};