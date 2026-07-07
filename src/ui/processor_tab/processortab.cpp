#include "processortab.h"
#include "circuit_scene.h"
#include "circuit_view.h"
#include "ui_processortab.h"
#include <QHBoxLayout>

// #include "vsrtl/graphics/vsrtl_widget.h"
namespace Kites
{
ProcessorTab::ProcessorTab(QWidget *parent, ProcessorManager *vmManager)
    : KitesTab(parent), ui(new Ui::ProcessorTab), m_vmManager(vmManager)
{
    ui->setupUi(this);
    m_vmStateTableModel = new VMStateTableModel(this, m_vmManager);
    ui->graphicsView->setScene(m_vmManager->getCircuitScene());
    ui->tableView->setModel(m_vmStateTableModel);
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void ProcessorTab::setWiresStayActive(bool stayActive)
{
    auto scene = m_vmManager->getCircuitScene();
    if (scene)
    {
        scene->setWireStayActive(stayActive);
    }
}

void ProcessorTab::onVMChanged()
{
    // The vm has been change so we ge the circuit scene of the
    // current vm and set it to the graphics view
    bool stayActive = m_vmManager->getCircuitScene()->getWireStayActive();
    ui->graphicsView->setScene(m_vmManager->getCircuitScene());
    m_vmManager->getCircuitScene()->setWireStayActive(stayActive);
    // keep the wire stay active setting
}

ProcessorTab::~ProcessorTab()
{
    delete ui;
}
} // namespace Kites
