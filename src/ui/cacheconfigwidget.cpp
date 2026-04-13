#include "ui/cacheconfigwidget.h"
#include "ui_cacheconfigwidget.h"
#include <QSignalBlocker>

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
    config.num_lines  = 1ULL << ui->linesSpinBox->value();
    config.block_size = 1ULL << ui->wordsSpinBox->value();
    config.num_ways = 1ULL << ui->waysSpinBox->value();
    config.write_policy = ui->writeHitComboBox->currentIndex() == 0 ? WritePolicy::WriteThrough : WritePolicy::WriteBack;
    config.allocation_policy = ui->writeMissComboBox->currentIndex() == 0 ? AllocationPolicy::WriteAllocate : AllocationPolicy::NoWriteAllocate;
    config.replacement_policy = ui->repPolComboBox->currentIndex() == 0 ? ReplacementPolicy::LRU : ReplacementPolicy::FIFO;
    return config;
}

int CacheConfigWidget::GetLinesExponent() const
{
    return ui->linesSpinBox->value();
}

int CacheConfigWidget::GetWaysExponent() const
{
    return ui->waysSpinBox->value();
}

int CacheConfigWidget::GetWordsExponent() const
{
    return ui->wordsSpinBox->value();
}

void CacheConfigWidget::SetLinesExponent(int value, bool notify)
{
    if (notify)
    {
        ui->linesSpinBox->setValue(value);
        return;
    }

    //blocking signal as we are changing the value programmatically 
    //and don't want to trigger configChanged signal
    QSignalBlocker blocker(ui->linesSpinBox);
    ui->linesSpinBox->setValue(value);
}

void CacheConfigWidget::SetWaysExponent(int value, bool notify)
{
    if (notify)
    {
        ui->waysSpinBox->setValue(value);
        return;
    }

    QSignalBlocker blocker(ui->waysSpinBox);
    ui->waysSpinBox->setValue(value);
}

void CacheConfigWidget::SetWordsExponent(int value, bool notify)
{
    if (notify)
    {
        ui->wordsSpinBox->setValue(value);
        return;
    }

    QSignalBlocker blocker(ui->wordsSpinBox);
    ui->wordsSpinBox->setValue(value);
}

CacheConfigWidget::~CacheConfigWidget()
{
    delete ui;
}
} // namespace Kites
