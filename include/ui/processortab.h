#ifndef PROCESSORTAB_H
#define PROCESSORTAB_H

#include "ui/kitestab.h"
#include "vm/vm_manager.h"
#include "ui/vm_state_table_model.h"
namespace Kites
{
namespace Ui {
class ProcessorTab;
}

class ProcessorTab : public KitesTab
{
    Q_OBJECT

public:
    // we are passing vmManager instead of just circuuit scene
    // because later we might want  to have acces to stuff like 
    // cpi ipc etc that are in vmManager
    explicit ProcessorTab(QWidget *parent = nullptr,VMManager* vmManager = nullptr);
    ~ProcessorTab();
    void setWiresStayActive(bool stayActive);
private:
    Ui::ProcessorTab *ui;
    VMManager* m_vmManager = nullptr;
    VMStateTableModel* m_vmStateTableModel;

public slots:
    void onVMChanged();
};
} // namespace Kites
#endif // PROCESSORTAB_H
