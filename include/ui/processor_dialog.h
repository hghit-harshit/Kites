#ifndef PROCESSOR_DIALOG_H
#define PROCESSOR_DIALOG_H

#include <QDialog>

namespace Kites
{
namespace Ui {
class ProcessorDialog;
}

class ProcessorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProcessorDialog(QWidget *parent = nullptr);
    ~ProcessorDialog();

private:
    Ui::ProcessorDialog *ui;
};
} // namespace Kites
#endif // PROCESSOR_DIALOG_H
