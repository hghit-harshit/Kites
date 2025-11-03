#include "ui/processor_designs/base_design.h"
#include "ui/processor_designs/components/alu_item.h"
#include "ui/processor_designs/components/rect_item.h"
namespace Kites
{
    BaseProcessorDesign::BaseProcessorDesign(QObject *parent)
    :QGraphicsScene(parent)
    {
        buildProcessor();
    }

    void BaseProcessorDesign::buildProcessor()
    {
        RectItem* instructionMemory = new RectItem("Instruction\nMemory");
        RectItem *registerFile = new RectItem("Registers");
        RectItem* dataMemory = new RectItem("Data\nMemory");
        ALUItem *alu = new ALUItem();
        alu->setPos(500, 250);
        instructionMemory->setPos(10,0);
        registerFile->setPos(50,0);
        dataMemory->setPos(110,0);
        addItem(alu);
        addItem(instructionMemory);
        addItem(registerFile);
        addItem(dataMemory);
       //m_components.insert("ALU", alu); // Add to map if you need to find it
    }
}