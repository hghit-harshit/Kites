#ifndef PROCESSORTAB_H
#define PROCESSORTAB_H

#include "ui/kitestab.h"
namespace Kites
{
namespace Ui {
class ProcessorTab;
}

class ProcessorTab : public KitesTab
{
    Q_OBJECT

public:
    explicit ProcessorTab(QWidget *parent = nullptr);
    ~ProcessorTab();

private:
    Ui::ProcessorTab *ui;
};
} // namespace Kites
#endif // PROCESSORTAB_H
