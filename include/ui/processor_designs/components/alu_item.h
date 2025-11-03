#pragma once
#include "base_component.h"

namespace Kites
{
class ALUItem : public BaseComponent
{
    public:
        ALUItem(QGraphicsItem* parent = nullptr)
        :BaseComponent
        ([](){
        QPainterPath aluPath;
        aluPath.moveTo(0, 0);
        aluPath.lineTo(40, 30);
        aluPath.lineTo(40, 70);
        aluPath.lineTo(0, 100);
        aluPath.lineTo(0, 60);
        aluPath.lineTo(10, 50);
        aluPath.lineTo(0, 40);
        //aluPath.lineTo(10, 100);
        aluPath.closeSubpath();
        return aluPath;
        }(),
        "ALU", parent)
    {}
};
}

//this will draw a structure like this
//        _______
//       |       \
//       |        \
//       |         |
//       |         | 
//       |         | 
//       |         |
//       |        /
//       |_______/