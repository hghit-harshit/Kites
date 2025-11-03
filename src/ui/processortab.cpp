#include "ui/processortab.h"
#include "ui_processortab.h"
#include "ui/processor_designs/base_design.h"
#include <QHBoxLayout>
//#include "vsrtl/graphics/vsrtl_widget.h"
namespace Kites{
ProcessorTab::ProcessorTab(QWidget *parent)
    : KitesTab(parent)
    , ui(new Ui::ProcessorTab)
{
    ui->setupUi(this);
    // Create your circuit design
    auto* scene = new BaseProcessorDesign();

    // Create the visualization widget
    //auto* vsrtlWidget = new vsrtl::VSRTLWidget(this);
    //vsrtlWidget->setDesign(design); // Important: builds the visual scen
    // Add it to your layout
    //QHBoxLayout *design_layout = new QHBoxLayout(ui->design_display);
    //design_layout->addWidget();
    ui->graphicsView->setScene(scene);
}

ProcessorTab::~ProcessorTab()
{
    delete ui;
}
}// namespace Kites
