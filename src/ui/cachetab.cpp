#include "ui/cachetab.h"
#include "ui_cachetab.h"
#include "ui/cacheconfigwidget.h"
#include "ui/cache_grid_delegate.h"
#include <algorithm>
namespace Kites
{
CacheTab::CacheTab(QWidget *parent,MemoryController* memoryController)
    : KitesTab(parent)
    , ui(new Ui::CacheTab)
{
    ui->setupUi(this);
    m_cacheModels.reserve(CacheLevel::CacheLevelCount);
    m_cacheModels[CacheLevel::Instruction] = new CacheModel(this,memoryController->GetInstructionCache());
    m_cacheModels[CacheLevel::L1] = new CacheModel(this,memoryController->GetL1Cache());
    m_cacheModels[CacheLevel::L2] = new CacheModel(this,memoryController->GetL2Cache());
    //ui->cacheTableView->setModel(m_L1cacheModel);
    //ui->cacheTableView->verticalHeader()->setVisible(false);
    //ui->cacheTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ui->splitter_2->setStretchFactor(1, 1);
    ui->splitter_3->setStretchFactor(1, 1);
    ui->splitter_4->setStretchFactor(1, 1);
    
    ui->L1tableView->setModel(m_cacheModels[CacheLevel::L1]);
    ui->L2tableView->setModel(m_cacheModels[CacheLevel::L2]);
    ui->InstructiontableView->setModel(m_cacheModels[CacheLevel::Instruction]);

    ui->L1tableView->verticalHeader()->setVisible(false);
    ui->L1tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->L2tableView->verticalHeader()->setVisible(false);
    ui->L2tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->InstructiontableView->verticalHeader()->setVisible(false);
    ui->InstructiontableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    auto* l1Delegate = new CacheGridDelegate(memoryController->GetL1Cache()->GetNumWays(), this);
    auto* l2Delegate = new CacheGridDelegate(memoryController->GetL2Cache()->GetNumWays(), this);
    auto* instructionDelegate = new CacheGridDelegate(memoryController->GetInstructionCache()->GetNumWays(), this);

    ui->L1tableView->setItemDelegate(l1Delegate);
    ui->L2tableView->setItemDelegate(l2Delegate);
    ui->InstructiontableView->setItemDelegate(instructionDelegate);

    ui->tabWidget->setTabText(0, "Instruction Cache");
    ui->tabWidget->setTabText(1, "L1 Cache");
    ui->tabWidget->setTabText(2, "L2 Cache");

    ui->tabWidget->setCurrentIndex(0); // show Instruction cache by default
    // ui->splitter->setStretchFactor(0, 0);
    // ui->splitter->setStretchFactor(1, 1);
    connectSignals("L1", ui->L1Config);
    connectSignals("L2", ui->L2Config);
    connectSignals("Instruction", ui->InstructionConfig);

    connect(memoryController->GetL1Cache(),&Cache::CacheStatsUpdatedSignal,ui->L1Config,CacheConfigWidget::CacheStatsUpdated);
    connect(memoryController->GetL2Cache(),&Cache::CacheStatsUpdatedSignal,ui->L2Config,CacheConfigWidget::CacheStatsUpdated);
    connect(memoryController->GetInstructionCache(),&Cache::CacheStatsUpdatedSignal,ui->InstructionConfig,CacheConfigWidget::CacheStatsUpdated);

    connect(ui->L1Config, &CacheConfigWidget::customPolicyScriptSelected, memoryController->GetL1Cache(), &Cache::LoadCustomPolicyScript);
    connect(ui->L2Config, &CacheConfigWidget::customPolicyScriptSelected, memoryController->GetL2Cache(), &Cache::LoadCustomPolicyScript);
    connect(ui->InstructionConfig, &CacheConfigWidget::customPolicyScriptSelected, memoryController->GetInstructionCache(), &Cache::LoadCustomPolicyScript);

    connect(memoryController->GetL1Cache(), &Cache::CustomPolicyScriptLoadedSignal, ui->L1Config, CacheConfigWidget::CustomPolicyScriptLoaded);
    connect(memoryController->GetL2Cache(), &Cache::CustomPolicyScriptLoadedSignal, ui->L2Config, CacheConfigWidget::CustomPolicyScriptLoaded);
    connect(memoryController->GetInstructionCache(), &Cache::CustomPolicyScriptLoadedSignal, ui->InstructionConfig, CacheConfigWidget::CustomPolicyScriptLoaded);

    connect(this, &CacheTab::cacheConfigChanged, memoryController, [memoryController, l1Delegate, l2Delegate, instructionDelegate](std::string cacheName, CacheConfig newConfig)
    {
        if(cacheName == "L1")
        {
            memoryController->GetL1Cache()->Reconfigure(newConfig);
            l1Delegate->UpdateNumWays(newConfig.num_ways);
        }
        else if(cacheName == "L2")
        {
            memoryController->GetL2Cache()->Reconfigure(newConfig);
            l2Delegate->UpdateNumWays(newConfig.num_ways);
        }
        else if(cacheName == "Instruction")
        {
            memoryController->GetInstructionCache()->Reconfigure(newConfig);
            instructionDelegate->UpdateNumWays(newConfig.num_ways);
        }
    });
   

}

void CacheTab::connectSignals(std::string cacheName, CacheConfigWidget* configWidget)
{
    auto reconfigure = [this,configWidget,cacheName](){
        if (m_enforcingConstraint)
        {
            return;
        }

        m_enforcingConstraint = true;

        if (cacheName == "L1")
        {
            emit cacheConfigChanged("L1", configWidget->GetConfig());
            if (enforceL2AtLeastL1())
            {
                emit cacheConfigChanged("L2", ui->L2Config->GetConfig());
            }
        }
        else if(cacheName == "Instruction")
        {
            emit cacheConfigChanged("Instruction", configWidget->GetConfig());
            if (enforceL2AtLeastL1())
            {
                emit cacheConfigChanged("L2", ui->L2Config->GetConfig());
            }
        }
        else
        {
            enforceL2AtLeastL1();
            emit cacheConfigChanged("L2", ui->L2Config->GetConfig());
        }

        m_enforcingConstraint = false;
    };

    connect(configWidget, &CacheConfigWidget::configChanged, this, reconfigure);

    
}

bool CacheTab::enforceL2AtLeastL1()
{
    const int l1Lines = ui->L1Config->GetLinesExponent();
    const int l1Ways = ui->L1Config->GetWaysExponent();
    const int l1Words = ui->L1Config->GetWordsExponent();

    const int instructionLines = ui->InstructionConfig->GetLinesExponent();
    const int instructionWays = ui->InstructionConfig->GetWaysExponent();
    const int instructionWords = ui->InstructionConfig->GetWordsExponent();

    const int l2Lines = ui->L2Config->GetLinesExponent();
    const int l2Ways = ui->L2Config->GetWaysExponent();
    const int l2Words = ui->L2Config->GetWordsExponent();

    const int newL2Lines = std::max({l2Lines, l1Lines, instructionLines});
    const int newL2Ways = std::max({l2Ways, l1Ways, instructionWays});
    const int newL2Words = std::max({l2Words, l1Words, instructionWords});

    const bool changed = (newL2Lines != l2Lines) || (newL2Ways != l2Ways) || (newL2Words != l2Words);
    if (!changed)
    {
        return false;
    }

    ui->L2Config->SetLinesExponent(newL2Lines, false);
    ui->L2Config->SetWaysExponent(newL2Ways, false);
    ui->L2Config->SetWordsExponent(newL2Words, false);
    return true;
}

CacheTab::~CacheTab()
{
    delete ui;
}
} // namespace Kites
