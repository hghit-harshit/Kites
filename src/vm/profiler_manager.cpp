#include "vm/profiler_manager.h"

ProfilerManager::ProfilerManager(QObject* parent)
    : QObject(parent)
{
}

void ProfilerManager::Reset()
{
    line_execution_counts_.clear();
    emit profilerReset();
}

void ProfilerManager::SetInstructionLineMapping(const std::map<unsigned int, unsigned int>& instructionToLineMapping)
{
    instruction_to_line_mapping_ = instructionToLineMapping;
    line_execution_counts_.clear();
    emit profilerReset();
}

void ProfilerManager::OnVMStateChanged(const QMap<QString, QVariant>& vmState)
{
    const int sourceLine = ResolveSourceLineFromState(vmState);
    if (sourceLine <= 0)
    {
        return;
    }

    ++line_execution_counts_[sourceLine];
    emit lineExecutionCountsUpdated(line_execution_counts_);
}

int ProfilerManager::ResolveSourceLineFromState(const QMap<QString, QVariant>& vmState) const
{
    // Primary path: use PC + assembler mapping for consistent results across VM variants.
    const QVariant pcVariant = vmState.value("ProgramCounter");
    bool ok = false;
    const qulonglong pc = pcVariant.toULongLong(&ok);
    if (ok)
    {
        const auto mappingIt = instruction_to_line_mapping_.find(static_cast<unsigned int>(pc / 4));
        if (mappingIt != instruction_to_line_mapping_.end())
        {
            return static_cast<int>(mappingIt->second);
        }
    }

    // Fallback for states where only EditorLines is populated.
    const QVariantMap editorLines = vmState.value("EditorLines").toMap();
    if (editorLines.contains("CI"))
    {
        bool ciOk = false;
        const int ciLine = editorLines.value("CI").toInt(&ciOk);
        if (ciOk)
        {
            return ciLine;
        }
    }

    if (editorLines.contains("."))
    {
        bool dotOk = false;
        const int dotLine = editorLines.value(".").toInt(&dotOk);
        if (dotOk)
        {
            return dotLine;
        }
    }

    return -1;
}
