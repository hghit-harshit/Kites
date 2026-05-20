#include "ui/cacheconfigwidget.h"
#include "ui_cacheconfigwidget.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSignalBlocker>

namespace Kites
{

CacheConfigWidget::CacheConfigWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::CacheConfigWidget)
{
    ui->setupUi(this);
    ui->sizeLineEdit->setReadOnly(true);
    ui->hitsLineEdit->setReadOnly(true);
    ui->missesLineEdit->setReadOnly(true);
    ui->writeBackLineEdit->setReadOnly(true);
    ui->hitrateLineEdit->setReadOnly(true);

    ui->customPolicyScriptlineEdit->setReadOnly(true);

    connect(ui->linesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &CacheConfigWidget::configChanged);
    connect(ui->waysSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &CacheConfigWidget::configChanged);
    connect(ui->wordsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &CacheConfigWidget::configChanged);

    connect(ui->linesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &CacheConfigWidget::UpdateSize);
    connect(ui->waysSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &CacheConfigWidget::UpdateSize);
    connect(ui->wordsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &CacheConfigWidget::UpdateSize);

    connect(ui->writeHitComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &CacheConfigWidget::configChanged);
    connect(ui->writeMissComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &CacheConfigWidget::configChanged);
    connect(ui->repPolComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this]
            {
                if (static_cast<ReplacementPolicy>(ui->repPolComboBox->currentIndex()) ==
                    ReplacementPolicy::Custom)
                {
                    OnCustomPolicyClicked();
                }
                else
                {
                    ui->customPolicyScriptlineEdit->clear();
                    m_lastSelectedPolicy =
                        static_cast<ReplacementPolicy>(ui->repPolComboBox->currentIndex());
                    emit CacheConfigWidget::configChanged();
                }
            });
    // connect(ui->browsePushButton, &QPushButton::clicked, this,
    // &CacheConfigWidget::OnBrowseClicked); connect(ui->applyPushButton, &QPushButton::clicked,
    // this, &CacheConfigWidget::onApplyClicked);
    UpdateSize();
}

void CacheConfigWidget::OnCustomPolicyClicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, tr("Select Custom Replacement Policy Script"), "", tr("lua Scripts (*.lua)"));
    if (!fileName.isEmpty())
    {
        // ui->customPolicyScriptlineEdit->setText(fileName);
        emit customPolicyScriptSelected(fileName.toStdString());
    }
    else
    {
        // If no file was selected, reset the combo box to a default value (e.g., LRU)
        QSignalBlocker blocker(ui->repPolComboBox);
        ui->repPolComboBox->setCurrentIndex(static_cast<int>(ReplacementPolicy::LRU));
        ui->customPolicyScriptlineEdit->clear();
        QMessageBox::warning(this, tr("No Script Selected"),
                             tr("No custom policy script was selected. Reverting to LRU."));
    }
}
//

CacheConfig CacheConfigWidget::GetConfig() const
{
    CacheConfig config;
    config.num_lines = 1ULL << ui->linesSpinBox->value();
    config.block_size = 1ULL << ui->wordsSpinBox->value();
    config.num_ways = 1ULL << ui->waysSpinBox->value();
    config.write_policy = ui->writeHitComboBox->currentIndex() == 0 ? WritePolicy::WriteThrough
                                                                    : WritePolicy::WriteBack;
    config.allocation_policy = ui->writeMissComboBox->currentIndex() == 0
                                   ? AllocationPolicy::WriteAllocate
                                   : AllocationPolicy::NoWriteAllocate;
    config.replacement_policy = static_cast<ReplacementPolicy>(ui->repPolComboBox->currentIndex());
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

    // blocking signal as we are changing the value programmatically
    // and don't want to trigger configChanged signal
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

void CacheConfigWidget::CacheStatsUpdated(CacheStats newStats)
{
    const size_t size = (size_t(1) << ui->linesSpinBox->value()) *
                        (size_t(1) << ui->wordsSpinBox->value()) *
                        (size_t(1) << ui->waysSpinBox->value());
    ui->sizeLineEdit->setText(QString::number(size) + " bytes");
    ui->hitsLineEdit->setText(QString::number(newStats.hits));
    ui->missesLineEdit->setText(QString::number(newStats.misses));
    ui->writeBackLineEdit->setText(QString::number(newStats.writeBacks));
    const size_t totalAccesses = newStats.hits + newStats.misses;
    const double hitrate =
        totalAccesses > 0 ? static_cast<double>(newStats.hits) / totalAccesses * 100.0 : 0.0;
    ui->hitrateLineEdit->setText(QString::number(hitrate, 'f', 2) + " %");
}

void CacheConfigWidget::CustomPolicyScriptLoaded(bool success, const std::string &message)
{
    QString messageStr = QString::fromStdString(message);
    if (success)
    {
        ui->customPolicyScriptlineEdit->setText(
            messageStr); // message contains the script path on success
    }
    else
    {
        ui->customPolicyScriptlineEdit->clear();
        QMessageBox::critical(this, tr("Custom Policy Load Failed"),
                              tr("Failed to load custom policy script: %1").arg(messageStr));
        QSignalBlocker blocker(ui->repPolComboBox);
        ui->repPolComboBox->setCurrentIndex(
            static_cast<int>(m_lastSelectedPolicy)); // revert to last selected policy on failure
    }
}

void CacheConfigWidget::UpdateSize()
{
    const size_t size = (size_t(1) << ui->linesSpinBox->value()) *
                        (size_t(1) << ui->wordsSpinBox->value()) *
                        (size_t(1) << ui->waysSpinBox->value());
    ui->sizeLineEdit->setText(QString::number(size) + " bytes");
}

CacheConfigWidget::~CacheConfigWidget()
{
    delete ui;
}
} // namespace Kites
