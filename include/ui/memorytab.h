#ifndef MEMORYTAB_H
#define MEMORYTAB_H

#include "ui/kitestab.h"

namespace Kites
{

namespace Ui {
class MemoryTab;
}

class MemoryTab : public KitesTab
{
    Q_OBJECT

public:
    explicit MemoryTab(QWidget *parent = nullptr);
    ~MemoryTab();

private:
    Ui::MemoryTab *ui;
};
}// namespace Kites
#endif // MEMORYTAB_H
