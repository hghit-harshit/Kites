#ifndef ADDPSEUDO_DIALOG_H
#define ADDPSEUDO_DIALOG_H

#include <QDialog>
namespace Kites
{
namespace Ui {
class AddPseudoDialog;
}

class AddPseudoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddPseudoDialog(QWidget *parent = nullptr);
    ~AddPseudoDialog();

private:
    void parsePseudoInstruction();
    Ui::AddPseudoDialog *ui;
};
}// namespace Kites
#endif // ADDPSEUDO_DIALOG_H
