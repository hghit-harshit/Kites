#include "ui/addpseudo_dialog.h"
#include "ui_addpseudo_dialog.h"

namespace Kites
{
AddPseudoDialog::AddPseudoDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddPseudoDialog)
{
    ui->setupUi(this);
}

AddPseudoDialog::~AddPseudoDialog()
{
    delete ui;
}

void AddPseudoDialog::parsePseudoInstruction()
{
    // we will parse the pseudo instruction details entered by the user in the dialog and add it to the custom pseudo instruction list
    // for now we will just print the entered details to the console
    QString pseudoInst = ui->pseudoInstructionTextEdit->toPlainText();
    QString expansion = ui->expansionTextEdit->toPlainText();

    pseudoInst = pseudoInst.trimmed();
    expansion = expansion.trimmed();

    qDebug() << "Pseudo Instruction: " << pseudoInst;
    qDebug() << "Expansion: " << expansion;
}
}// namespace Kites
