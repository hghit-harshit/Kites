#pragma once
#include "ui/kitestab.h"
#include "ui/cachemodel.h"
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
    CacheModel* m_cacheModel = nullptr;
    Ui::CacheTab *ui;
};
}
