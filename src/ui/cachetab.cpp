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
    m_cacheModel = new CacheModel(this,memoryController);
    ui->cacheTableView->setModel(m_cacheModel);
    ui->cacheTableView->verticalHeader()->setVisible(false);
    ui->cacheTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ui->tabWidget->setTabText(0, "L1 Cache");
    ui->tabWidget->setTabText(1, "L2 Cache");

    //CacheConfigWidget* configWidget = new CacheConfigWidget(this);





    // auto *header = ui->cacheTableView->horizontalHeader();
    // header->setSectionResizeMode(0, QHeaderView::ResizeToContents); // Index
    // header->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Valid
    // header->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Dirty
    // header->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Tag (optional)

    // // Make Data column take remaining space
    // header->setSectionResizeMode(4, QHeaderView::Stretch);
}

CacheTab::~CacheTab()
{
    delete ui;
}
} // namespace Kites
