#include "profilertab.h"
#include "ui_profilertab.h"
#include "profiler/profiler.h"
#include "common/instruction_types.h"
#include "instruction_type_count_model.h"
#include <QLineEdit>

namespace Kites
{
ProfilerTab::ProfilerTab(QWidget *parent, const Profiler *profiler)
    : KitesTab(parent), ui(new Ui::ProfilerTab), m_profiler(profiler)
{
    ui->setupUi(this);
    ui->splitter->setStretchFactor(0, 1);
    
    ui->instructionTypeCountTableView->horizontalHeader()->
    setSectionResizeMode(QHeaderView::Stretch);
    ui->instructionTypeCountTableView->verticalHeader()->setVisible(false);
    if (m_profiler)
    {   
        InstructionTypeCountModel *model = new InstructionTypeCountModel(this, 
        m_profiler->getInstructionTypeCounts());
        ui->instructionTypeCountTableView->setModel(model);
        // connect(m_profiler, &Profiler::lineExecutionCountsUpdated, this,
        //         &ProfilerTab::updateLineExecutionCounts);
        connect(m_profiler, &Profiler::profilerReset, this,
                &ProfilerTab::resetProfilerView);
        connect(m_profiler, &Profiler::incrementLineExecutionCountSignal, ui->plainTextEdit,
                &ProfilerEditor::incrementLineExecutionCountSlot);
        connect(m_profiler, &Profiler::updateLineInstructionTypeSignal, ui->plainTextEdit,
                &ProfilerEditor::updateLineInstructionTypeSlot);
        connect(m_profiler, &Profiler::updateInstructionTypeCountsSignal, model,
                &InstructionTypeCountModel::updateInstructionTypeCountsSlot);
    }
}

ProfilerTab::~ProfilerTab()
{
    delete ui;
}

void ProfilerTab::setSourceText(const QString &sourceText)
{
    ui->plainTextEdit->setPlainText(sourceText);
    ui->plainTextEdit->clearExecutionCount();
}

void ProfilerTab::updateLineExecutionCount(const std::map<int, int> &lineExecutionCounts)
{
    ui->plainTextEdit->setExecutionCount(lineExecutionCounts);
}

void ProfilerTab::updateLineInstructionType(const std::map<int, std::string> &lineInstructionTypes)
{
    ui->plainTextEdit->setInstructionTypes(lineInstructionTypes);
}

void ProfilerTab::updateStatistics(const std::map<std::string, int> &statistics)
{
    
}

void ProfilerTab::resetProfilerView()
{

}
} // namespace Kites
