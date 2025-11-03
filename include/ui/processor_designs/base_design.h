#pragma once
/* #include "vsrtl/core/vsrtl_adder.h"
#include "vsrtl/core/vsrtl_constant.h"
#include "vsrtl/core/vsrtl_design.h"
#include "vsrtl/core/vsrtl_logicgate.h"
#include "vsrtl/core/vsrtl_multiplexer.h"
#include "vsrtl/core/vsrtl_alu.h"
#include "vsrtl/interface/vsrtl_interface.h"
//#include "vsrtl/core/"
/**
 * @brief This is the class from which the design for each processor(vm) 
 * will inherit from 
 * 
 */
// fuck this vsrtl imma do my own thing
#include <QGraphicsView>
#include <QGraphicsScene>
namespace Kites
{
class BaseProcessorDesign : public QGraphicsScene
{
    Q_OBJECT
    public : 
        explicit BaseProcessorDesign(QObject* parent = nullptr);

    public slots:
        // well add slots to change wire state later
        // later means in like 2 days
    private:
        void buildProcessor(); // later well make it virtual but just testing for now 

};
}