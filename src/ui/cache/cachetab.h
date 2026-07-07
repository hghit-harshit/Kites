#pragma once
#include "cache_grid_delegate.h"
#include "cacheconfigwidget.h"
#include "cachemodel.h"
#include "ui/common/kitestab.h"
#include "processor/memory_controller.h"
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
    enum CacheLevel
    {
        L1 = 0,
        L2,
        Instruction,
        CacheLevelCount
    };
	//void updateCacheConfig(CacheLevel cacheLevel, CacheConfig newConfig);
    bool enforceL2AtLeastL1();

    std::array<CacheModel*, CacheLevelCount> m_cacheModels{};
    MemoryController  *m_memoryController    {nullptr};
    Ui::CacheTab *ui;
signals:
    void cacheConfigChangedSignal(CacheLevel cacheLevel, CacheConfig newConfig);
};
} // namespace Kites
