#include "ui/addpseudo_dialog.h"
#include "assembler/custom_pseudo_manager.h"
#include "ui_addpseudo_dialog.h"
#include <QMessageBox>
#include <QPushButton>

namespace Kites
{
AddPseudoDialog::AddPseudoDialog(QWidget *parent, bool isUpdate)
    : QDialog(parent), ui(new Ui::AddPseudoDialog), m_isUpdate(isUpdate)
{
    ui->setupUi(this);
    connect(ui->saveButton, &QPushButton::clicked, this, &AddPseudoDialog::parsePseudoInstruction);
}

AddPseudoDialog::~AddPseudoDialog()
{
    delete ui;
}

QString AddPseudoDialog::getPseudoInstruction() const
{
    return ui->pseudoInstructionTextEdit->toPlainText().trimmed();
}

QString AddPseudoDialog::getExpansion() const
{
    return ui->expansionTextEdit->toPlainText().trimmed();
}

void AddPseudoDialog::setPseudoInstruction(const QString &pseudoInst)
{
    ui->pseudoInstructionTextEdit->setPlainText(pseudoInst);
}

void AddPseudoDialog::setExpansion(const QString &expansion)
{
    ui->expansionTextEdit->setPlainText(expansion);
}

void AddPseudoDialog::parsePseudoInstruction()
{
    // we will parse the pseudo instruction details entered by the user in the dialog and add it to
    // the custom pseudo instruction list for now we will just print the entered details to the
    // console
    QString pseudoInst = ui->pseudoInstructionTextEdit->toPlainText().trimmed();
    QString expansion = ui->expansionTextEdit->toPlainText().trimmed();

    if (pseudoInst.isEmpty() || expansion.isEmpty())
    {
        QMessageBox::critical(this, "Invalid Input",
                              "Please enter both pseudo instruction and its expansion.");
        return;
    }

    QString errorMessage;
    if (!CustomPseudoManager::addCustomPseudoInstruction(getPseudoInstruction(), getExpansion(),
                                                         errorMessage, m_isUpdate))
    {
        QMessageBox::critical(nullptr, "Error", errorMessage);
        return;
    }

    accept();
}
} // namespace Kites
