#include "ui/processortab.h"
#include "ui_processortab.h"
namespace Kites{
ProcessorTab::ProcessorTab(QWidget *parent)
    : KitesTab(parent)
    , ui(new Ui::ProcessorTab)
{
    ui->setupUi(this);
}

ProcessorTab::~ProcessorTab()
{
    delete ui;
}
}// namespace Kites
