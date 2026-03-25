#ifndef COMPILERTAB_H
#define COMPILERTAB_H

#include <QWidget>
#include "kitestab.h"

namespace Kites
{
namespace Ui {
class CompilerTab;
}

class CompilerTab : public KitesTab
{
    Q_OBJECT

public:
    explicit CompilerTab(QWidget *parent = nullptr);
    ~CompilerTab();
    void convertToAssemblyClicked();
private:
    Ui::CompilerTab *ui;
    QString cleanAssembly(const QString &rawAssembly);
};
}
#endif // COMPILERTAB_H
