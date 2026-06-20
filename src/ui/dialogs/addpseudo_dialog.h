#ifndef ADDPSEUDO_DIALOG_H
#define ADDPSEUDO_DIALOG_H

#include <QDialog>
namespace Kites
{
namespace Ui
{
class AddPseudoDialog;
}

class AddPseudoDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit AddPseudoDialog(QWidget *parent = nullptr, bool isUpdate = false);
    ~AddPseudoDialog();
    QString getPseudoInstruction() const;
    QString getExpansion() const;

    void setPseudoInstruction(const QString &pseudoInst);
    void setExpansion(const QString &expansion);

  private:
    void parsePseudoInstruction();
    Ui::AddPseudoDialog *ui;
    bool m_isUpdate = false;
};
} // namespace Kites
#endif // ADDPSEUDO_DIALOG_H
