#ifndef COMPILERTAB_H
#define COMPILERTAB_H

#include "ui/common/kitestab.h"
#include <QWidget>


namespace Kites
{
namespace Ui
{
class CompilerTab;
}

class CompilerTab : public KitesTab
{
    Q_OBJECT

  public:
    explicit CompilerTab(QWidget *parent = nullptr);
    ~CompilerTab();
    QString getUsersCompilerOptions();
    void onCopyToMainEditorClicked();
    void onConvertToAssemblyClicked();

  private slots:
    void applyOutputBorderStyle();

  private:
    Ui::CompilerTab *ui;
    QString cleanAssembly(const QString &rawAssembly);
};
} // namespace Kites
#endif // COMPILERTAB_H
