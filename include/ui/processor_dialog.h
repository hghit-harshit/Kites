#ifndef PROCESSOR_DIALOG_H
#define PROCESSOR_DIALOG_H

#include <QDialog>
#include "vm/vm_types.h"
#include <QTreeWidgetItem>
namespace Kites
{
namespace Ui {
class ProcessorDialog;
}

class ProcessorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProcessorDialog(QWidget *parent = nullptr,const VMType& currentVMType = VMType::RVSS);
    ~ProcessorDialog();

private:
    Ui::ProcessorDialog *ui;
    QTreeWidgetItem* m_currentSelectedItem = nullptr;
private slots:
    void on_buttonBox_accepted();
    //void on_treeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

signals:
    void vmSelected(const VMType& vmType);
};
} // namespace Kites
#endif // PROCESSOR_DIALOG_H
