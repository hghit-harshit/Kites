#pragma once
#include <QGraphicsPathItem>
#include <QPainterPath>

namespace Kites
{
class BaseComponent : public QGraphicsPathItem
{
public:
    BaseComponent(const QPainterPath &path, const QString &name, QGraphicsItem *parent = nullptr);
    void setName(const QString& name);
    virtual QJsonObject toJson();
private:
    QGraphicsTextItem *m_label;
    QString m_name;
    
};
} // namespace Kites