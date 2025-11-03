#pragma once
#include "base_component.h"

namespace Kites
{
/**
 * @brief This class will be used to draw registerfile and memories 
 * as then are rectangular
 * 
 */
class RectItem : public BaseComponent
{
    public:
        RectItem(const QString& name,QGraphicsItem *parent = nullptr)
        :BaseComponent
        ([](){
        QPainterPath aluPath;
        aluPath.moveTo(0, 0);
        aluPath.lineTo(40, 0);
        aluPath.lineTo(40, 100);
        aluPath.lineTo(0, 100);
        // aluPath.lineTo(0, 60);
        // aluPath.lineTo(10, 50);
        // aluPath.lineTo(0, 40);
        //aluPath.lineTo(10, 100);
        aluPath.closeSubpath();
        return aluPath;
        }(),
        name, parent){}
};
}