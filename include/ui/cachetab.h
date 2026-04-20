#pragma once
#include "ui/kitestab.h"
#include "ui/cachemodel.h"
#include "ui/cacheconfigwidget.h"
#include "vm/memory_controller.h"
#include <QWidget>

namespace Kites
{
namespace Ui {
class CacheTab;
}

class CacheTab : public KitesTab
{
    Q_OBJECT

public:
    explicit CacheTab(QWidget *parent = nullptr,MemoryController* memoryController = nullptr );
    ~CacheTab();

private:

    enum CacheLevel : size_t
    {
        L1 =0,
        L2 ,
        Instruction,
        CacheLevelCount
    };
    void connectSignals(std::string cacheName, CacheConfigWidget* configWidget);
    bool enforceL2AtLeastL1();

    std::vector<CacheModel*> m_cacheModels{};
    

    bool m_enforcingConstraint = false;
    Ui::CacheTab *ui;
signals:
    void cacheConfigChanged(std::string cacheName, CacheConfig newConfig);
};
} // namespace Kites
