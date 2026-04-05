#include "ui/cachetab.h"
#include "ui_cachetab.h"
#include "ui/cacheconfigwidget.h"
namespace Kites
{
CacheTab::CacheTab(QWidget *parent,MemoryController* memoryController)
    : KitesTab(parent)
    , ui(new Ui::CacheTab)
{
    ui->setupUi(this);
    m_L1cacheModel = new CacheModel(this,memoryController->GetL1Cache());
    m_L2cacheModel = new CacheModel(this,memoryController->GetL2Cache());
    //ui->cacheTableView->setModel(m_L1cacheModel);
    //ui->cacheTableView->verticalHeader()->setVisible(false);
    //ui->cacheTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->L1tableView->setModel(m_L1cacheModel);
    ui->L2tableView->setModel(m_L2cacheModel); 

    ui->L1tableView->verticalHeader()->setVisible(false);
    ui->L1tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->L2tableView->verticalHeader()->setVisible(false);
    ui->L2tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ui->tabWidget->setTabText(0, "L1 Cache");
    ui->tabWidget->setTabText(1, "L2 Cache");

    ui->tabWidget->setCurrentIndex(0); // show L1 cache by default

    connectSignals("L1", ui->L1Config);
    connectSignals("L2", ui->L2Config);

    connect(this, &CacheTab::cacheConfigChanged, memoryController, [memoryController](std::string cacheName, CacheConfig newConfig){
        if(cacheName == "L1")
        {
            memoryController->GetL1Cache()->Reconfigure(newConfig);
        }
        else if(cacheName == "L2")
        {
            memoryController->GetL2Cache()->Reconfigure(newConfig);
        }
    });
    // connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int index){
    //     if(index == 0)
    //     {
    //         ui->cacheTableView->setModel(m_L1cacheModel);
    //         m_L1cacheModel->updateCacheData(0); // update entire model for now
    //     }
    //     else if(index == 1)
    //     {
    //         ui->cacheTableView->setModel(m_L2cacheModel);
    //         m_L2cacheModel->updateCacheData(0); // update entire model for now
    //     }
    // });

}

void CacheTab::connectSignals(std::string cacheName, CacheConfigWidget* configWidget)
{
    auto reconfigure = [this,configWidget,cacheName](){
        CacheConfig newConfig = configWidget->GetConfig();
        emit cacheConfigChanged(cacheName, newConfig);
    };

    connect(configWidget, &CacheConfigWidget::configChanged, this, reconfigure);
}

CacheTab::~CacheTab()
{
    delete ui;
}
} // namespace Kites
