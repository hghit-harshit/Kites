#include "ui/profilertab.h"
#include "ui_profilertab.h"
#include "vm/profiler_manager.h"

#include <QLineEdit>

namespace Kites
{
ProfilerTab::ProfilerTab(QWidget *parent, ProfilerManager *profilerManager)
    : KitesTab(parent), ui(new Ui::ProfilerTab), profiler_manager_(profilerManager)
{
    ui->setupUi(this);
    ui->splitter->setStretchFactor(0, 1);

    if (profiler_manager_)
    {
        connect(profiler_manager_, &ProfilerManager::lineExecutionCountsUpdated, this,
                &ProfilerTab::updateLineExecutionCounts);
        connect(profiler_manager_, &ProfilerManager::profilerReset, this,
                &ProfilerTab::resetProfilerView);
    }
}

ProfilerTab::~ProfilerTab()
{
    delete ui;
}

void ProfilerTab::setSourceText(const QString &sourceText)
{
    ui->plainTextEdit->setPlainText(sourceText);
    ui->plainTextEdit->clearHitCount();
}

void ProfilerTab::updateLineExecutionCounts(const std::map<int, int> &lineExecutionCounts)
{
    ui->plainTextEdit->setHitCount(lineExecutionCounts);
}

void ProfilerTab::updateInstructionTypes(const std::map<int, std::string> &instructionTypes)
{
    /*TODO move this logic to the profiler manager but this will have to do for now*/
    ui->plainTextEdit->setInstructionTypes(instructionTypes);

    int rTypeCount = 0;
    int iTypeCount = 0;
    int sTypeCount = 0;
    int jTypeCount = 0;
    int bTypeCount = 0;
    int uTypeCount = 0;

    for (const auto &[lineNumber, type] : instructionTypes)
    {
        Q_UNUSED(lineNumber);

        if (type == "R-Type")
        {
            ++rTypeCount;
        }
        else if (type == "I1-Type" || type == "I2-Type" || type == "I3-Type" || type == "I-Type")
        {
            ++iTypeCount;
        }
        else if (type == "J-Type")
        {
            ++jTypeCount;
        }
        else if (type == "S-Type")
        {
            ++sTypeCount;
        }
        else if (type == "B-Type")
        {
            ++bTypeCount;
        }
        else if (type == "U-Type")
        {
            ++uTypeCount;
        }
    }

    ui->RTypelineEdit->setText(QString::number(rTypeCount));
    ui->ITypelineEdit->setText(QString::number(iTypeCount));
    ui->JTypelineEdit->setText(QString::number(jTypeCount));
    ui->BTypelineEdit->setText(QString::number(bTypeCount));

    if (auto *sTypeLineEdit = findChild<QLineEdit *>("STypelineEdit"))
    {
        sTypeLineEdit->setText(QString::number(sTypeCount));
    }
    if (auto *uTypeLineEdit = findChild<QLineEdit *>("UTypelineEdit"))
    {
        uTypeLineEdit->setText(QString::number(uTypeCount));
    }
}

void ProfilerTab::updateStatistics(const std::map<std::string, int> &statistics)
{
    // const auto getOrDefault = [&statistics](const char *key) -> int
    // {
    //     const auto it = statistics.find(key);
    //     return it != statistics.end() ? it->second : 0;
    // };

    // ui->CycleslineEdit->setText(QString::number(getOrDefault("Cycles")));
    // ui->IPClineEdit->setText(QString::number(getOrDefault("IPC")));
    // ui->CPIlineEdit->setText(QString::number(getOrDefault("CPI")));
    // ui->InstlineEdit->setText(QString::number(getOrDefault("InstRetired")));
}

void ProfilerTab::resetProfilerView()
{
    ui->plainTextEdit->clearHitCount();
    ui->RTypelineEdit->clear();
    ui->ITypelineEdit->clear();
    ui->JTypelineEdit->clear();
    ui->BTypelineEdit->clear();

    if (auto *sTypeLineEdit = findChild<QLineEdit *>("STypelineEdit"))
    {
        sTypeLineEdit->clear();
    }
    if (auto *uTypeLineEdit = findChild<QLineEdit *>("UTypelineEdit"))
    {
        uTypeLineEdit->clear();
    }
    // ui->CycleslineEdit->clear();
    // ui->IPClineEdit->clear();
    // ui->CPIlineEdit->clear();
    // ui->InstlineEdit->clear();
}
} // namespace Kites
