#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QString>
namespace Kites
{
namespace Ui
{
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

  private:
    Ui::SettingsDialog *ui;

    QWidget *createCustomPseudoInstPage();
    QWidget *createEditorPage();

    void addPage(const QString &name, QWidget *page);

    std::vector<std::pair<QString, int>> m_pages;
};
} // namespace Kites
#endif // SETTINGS_DIALOG_H
