#include "ui/processor_dialog.h"
#include "ui_processor_dialog.h"

namespace Kites
{
ProcessorDialog::ProcessorDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ProcessorDialog)
{
    ui->setupUi(this);
    ui->treeWidget->setHeaderHidden(true);
}

ProcessorDialog::~ProcessorDialog()
{
    delete ui;
}
} //namespace Kites
