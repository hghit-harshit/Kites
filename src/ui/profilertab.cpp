#include "ui/profilertab.h"
#include "ui_profilertab.h"
#include "vm/profiler_manager.h"

namespace Kites
{
ProfilerTab::ProfilerTab(QWidget *parent, ProfilerManager* profilerManager)
    : KitesTab(parent)
    , ui(new Ui::ProfilerTab)
    , profiler_manager_(profilerManager)
{
    ui->setupUi(this);

    if (profiler_manager_)
    {
        connect(profiler_manager_, &ProfilerManager::lineExecutionCountsUpdated,
                this, &ProfilerTab::updateLineExecutionCounts);
        connect(profiler_manager_, &ProfilerManager::profilerReset,
                this, &ProfilerTab::resetProfilerView);
    }
}

ProfilerTab::~ProfilerTab()
{
    delete ui;
}

void ProfilerTab::setSourceText(const QString& sourceText)
{
    ui->plainTextEdit->setPlainText(sourceText);
    ui->plainTextEdit->clearHitCount();
}

void ProfilerTab::updateLineExecutionCounts(const std::map<int, int>& lineExecutionCounts)
{
    ui->plainTextEdit->setHitCount(lineExecutionCounts);
}

void ProfilerTab::resetProfilerView()
{
    ui->plainTextEdit->clearHitCount();
}
}// namespace Kites
