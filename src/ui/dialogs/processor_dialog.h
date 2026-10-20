#ifndef PROCESSOR_DIALOG_H
#define PROCESSOR_DIALOG_H

#include "processor/processor_types.h"
#include <QDialog>

namespace Kites
{
namespace Ui
{
class ProcessorDialog;
}

class ProcessorDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit ProcessorDialog(QWidget *parent = nullptr, const ProcessorType &currentVMType = ProcessorType::RVSS);
    ~ProcessorDialog();

  private:
    Ui::ProcessorDialog *ui;
    void setPipelineOptionsEnabled(bool enabled);
  private slots:
    void on_buttonBox_accepted();

  signals:
    void vmSelected(const ProcessorType &vmType);
};
} // namespace Kites
#endif // PROCESSOR_DIALOG_H
