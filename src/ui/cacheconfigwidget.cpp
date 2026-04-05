#include "ui/cacheconfigwidget.h"
#include "ui_cacheconfigwidget.h"

namespace Kites
{

CacheConfigWidget::CacheConfigWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CacheConfigWidget)
{
    ui->setupUi(this);
    connect(ui->linesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CacheConfigWidget::configChanged);
    connect(ui->waysSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CacheConfigWidget::configChanged);
    connect(ui->wordsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CacheConfigWidget::configChanged);
    connect(ui->writeHitComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CacheConfigWidget::configChanged);
    connect(ui->writeMissComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CacheConfigWidget::configChanged);
    connect(ui->repPolComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CacheConfigWidget::configChanged);
}

CacheConfig CacheConfigWidget::GetConfig() const
{
    CacheConfig config;
    config.num_lines = ui->linesSpinBox->value();
    config.block_size = ui->wordsSpinBox->value();
    config.num_ways = ui->waysSpinBox->value();
    config.write_policy = ui->writeHitComboBox->currentIndex() == 0 ? WritePolicy::WriteThrough : WritePolicy::WriteBack;
    config.allocation_policy = ui->writeMissComboBox->currentIndex() == 0 ? AllocationPolicy::WriteAllocate : AllocationPolicy::NoWriteAllocate;
    config.replacement_policy = ui->repPolComboBox->currentIndex() == 0 ? ReplacementPolicy::LRU : ReplacementPolicy::FIFO;
    return config;
}

CacheConfigWidget::~CacheConfigWidget()
{
    delete ui;
}
} // namespace Kites