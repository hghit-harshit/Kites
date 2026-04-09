#ifndef PROFILERTAB_H
#define PROFILERTAB_H

#include <QWidget>
#include "kitestab.h"
namespace Kites
{
namespace Ui {
class ProfilerTab;
}

class ProfilerTab : public KitesTab
{
    Q_OBJECT

public:
    explicit ProfilerTab(QWidget *parent = nullptr);
    ~ProfilerTab();

private:
    Ui::ProfilerTab *ui;
};
}// namespace Kites
#endif // PROFILERTAB_H
