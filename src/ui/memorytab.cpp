#include "ui/memorytab.h"
#include "ui_memorytab.h"
namespace Kites
{


MemoryTab::MemoryTab(QWidget *parent)
    : KitesTab(parent)
    , ui(new Ui::MemoryTab)
{
    ui->setupUi(this);
}

MemoryTab::~MemoryTab()
{
    delete ui;
}
} // namespaec Kites
