#include "processor_dialog.h"
#include "ui_processor_dialog.h"

#include <QComboBox>

namespace Kites
{
ProcessorDialog::ProcessorDialog(QWidget *parent, const ProcessorType &currentVMType)
    : QDialog(parent), ui(new Ui::ProcessorDialog)
{
    ui->setupUi(this);

    const bool isPipeline = currentVMType != ProcessorType::RVSS;
    ui->processorComboBox->setCurrentIndex(isPipeline ? 1 : 0);

    switch (currentVMType)
    {
        case ProcessorType::RV5Stage_H_NF:
            ui->hazardDetectionCheckBox->setChecked(true);
            break;
        case ProcessorType::RV5Stage_NH_F:
            ui->forwardingCheckBox->setChecked(true);
            break;
        case ProcessorType::RV5Stage_H_F:
            ui->hazardDetectionCheckBox->setChecked(true);
            ui->forwardingCheckBox->setChecked(true);
            break;
        default:
            break;
    }

    setPipelineOptionsEnabled(isPipeline);
    connect(ui->processorComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int index) { setPipelineOptionsEnabled(index == 1); });
}

ProcessorDialog::~ProcessorDialog()
{
    delete ui;
}

void ProcessorDialog::on_buttonBox_accepted()
{
    if (ui->processorComboBox->currentIndex() == 0)
    {
        emit vmSelected(ProcessorType::RVSS);
        return;
    }

    const bool hazardDetection = ui->hazardDetectionCheckBox->isChecked();
    const bool forwarding = ui->forwardingCheckBox->isChecked();
    if (hazardDetection && forwarding)
        emit vmSelected(ProcessorType::RV5Stage_H_F);
    else if (hazardDetection)
        emit vmSelected(ProcessorType::RV5Stage_H_NF);
    else if (forwarding)
        emit vmSelected(ProcessorType::RV5Stage_NH_F);
    else
        emit vmSelected(ProcessorType::RV5Stage_NH_NF);
}

void ProcessorDialog::setPipelineOptionsEnabled(bool enabled)
{
    // Keep the container enabled. Disabling a styled QGroupBox can leave its
    // child controls looking disabled after the pipeline mode is reselected.
    ui->hazardDetectionCheckBox->setEnabled(enabled);
    ui->forwardingCheckBox->setEnabled(enabled);
    ui->pipelineOptionsHintLabel->setText(
        enabled ? "Configure how the 5-stage pipeline handles data dependencies."
                : "Available only when 5-stage pipeline is selected.");

}

} // namespace Kites
