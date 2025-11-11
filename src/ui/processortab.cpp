#include "ui/processortab.h"
#include "ui_processortab.h"
#include "ui/circuit_view.h"
#include "ui/circuit_scene.h"
#include <QHBoxLayout>
//#include "vsrtl/graphics/vsrtl_widget.h"
namespace Kites
{
ProcessorTab::ProcessorTab(QWidget *parent,VMManager* vmManager)
    : KitesTab(parent)
    , ui(new Ui::ProcessorTab)
    ,m_vmManager(vmManager)
{
    ui->setupUi(this);
    // Create your circuit design
    //auto* scene = new BaseProcessorDesign();

    // Create the visualization widget
    //auto* vsrtlWidget = new vsrtl::VSRTLWidget(this);
    //vsrtlWidget->setDesign(design); // Important: builds the visual scen
    // Add it to your layout
    //QHBoxLayout *design_layout = new QHBoxLayout(ui->design_display);
    //design_layout->addWidget();
    //auto *scene = new CircuitScene(this);
    //scene->loadScene("C:/Users/hghit/Desktop/NH_NF_Processor.json");
    ui->graphicsView->setScene(m_vmManager->getCircuitScene());
    
}

void ProcessorTab::onVMChanged()
{
    // The vm has been change so we ge the circuit scene of the 
    // current vm and set it to the graphics view
    ui->graphicsView->setScene(m_vmManager->getCircuitScene());
}

ProcessorTab::~ProcessorTab()
{
    delete ui;
}
}// namespace Kites
