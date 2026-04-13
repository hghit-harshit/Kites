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
    void connectSignals(std::string cacheName, CacheConfigWidget* configWidget);
    bool enforceL2AtLeastL1();
    CacheModel* m_L1cacheModel = nullptr;
    CacheModel* m_L2cacheModel = nullptr;
    bool m_enforcingConstraint = false;
    Ui::CacheTab *ui;
signals:
    void cacheConfigChanged(std::string cacheName, CacheConfig newConfig);
};
}
