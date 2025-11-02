#ifndef MEMORYTAB_H
#define MEMORYTAB_H

#include "ui/kitestab.h"
#include "ui/memorymodel.h"
namespace Kites
{

namespace Ui {
class MemoryTab;
}

class MemoryTab : public KitesTab
{   
    Q_OBJECT

public:
    explicit MemoryTab(QWidget *parent = nullptr,MemoryController* memoryController = nullptr);
    void changeMemoryController(MemoryController* memoryController);
    ~MemoryTab();

private:
    Ui::MemoryTab *ui;
    MemoryModel* m_memoryModel = nullptr;
};
}// namespace Kites
#endif // MEMORYTAB_H
