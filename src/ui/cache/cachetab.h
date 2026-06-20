#pragma once
#include "cache_grid_delegate.h"
#include "cacheconfigwidget.h"
#include "cachemodel.h"
#include "ui/common/kitestab.h"
#include "vm/memory_controller.h"
#include <QWidget>


namespace Kites
{
namespace Ui
{
class CacheTab;
}

class CacheTab : public KitesTab
{
    Q_OBJECT

  public:
    explicit CacheTab(QWidget *parent = nullptr, MemoryController *memoryController = nullptr);
    ~CacheTab();
    void changeMemoryController(MemoryController *memoryController);

  private:
    enum CacheLevel : size_t
    {
        L1 = 0,
        L2,
        Instruction,
        CacheLevelCount
    };
    void connectSignals(std::string cacheName, CacheConfigWidget *configWidget);
    bool enforceL2AtLeastL1();

    std::vector<CacheModel*> m_cacheModels{};
    MemoryController  *m_memoryController    = nullptr;
    CacheGridDelegate *m_l1Delegate          = nullptr;
    CacheGridDelegate *m_l2Delegate          = nullptr;
    CacheGridDelegate *m_instructionDelegate = nullptr;

    bool m_enforcingConstraint = false;
    Ui::CacheTab *ui;
  signals:
    void cacheConfigChanged(std::string cacheName, CacheConfig newConfig);
};
} // namespace Kites
