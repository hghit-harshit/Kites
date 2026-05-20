#include "ui/processor_dialog.h"
#include "ui_processor_dialog.h"

namespace Kites
{
ProcessorDialog::ProcessorDialog(QWidget *parent, const VMType &currentVMType)
    : QDialog(parent), ui(new Ui::ProcessorDialog)
{
    ui->setupUi(this);
    ui->treeWidget->setHeaderHidden(true);
    ui->treeWidget->setCurrentItem(ui->treeWidget->topLevelItem(static_cast<int>(currentVMType)));
}

ProcessorDialog::~ProcessorDialog()
{
    delete ui;
}

void ProcessorDialog::on_buttonBox_accepted()
{
    QTreeWidgetItem *selectedItem = ui->treeWidget->currentItem();
    if (selectedItem)
    {
        m_currentSelectedItem = selectedItem;
        QString processorType = selectedItem->text(0);
        // all these names were set in processor_dialog.ui using qt Designer
        if (processorType == "Single cycle processor")
        {
            emit vmSelected(VMType::RVSS);
        }
        else if (processorType == "5 statge Processor w/o hazard detection w/o forwarding")
        {
            emit vmSelected(VMType::RV5Stage_NH_NF);
        }
        else if (processorType == "5 statge Processor w/ hazard detection w/o forwarding")
        {
            emit vmSelected(VMType::RV5Stage_H_NF);
        }
        else if (processorType == "5 statge Processor w/o hazard detection w/ forwarding")
        {
            emit vmSelected(VMType::RV5Stage_NH_F);
        }
        else if (processorType == "5 statge Processor w/ hazard detection w/ forwarding")
        {
            emit vmSelected(VMType::RV5Stage_H_F);
        }
    }
}

} // namespace Kites
