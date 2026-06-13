#include "vm/profiler.h"
#include "common/instructions.h"


Profiler::Profiler(QObject *parent) : QObject(parent)
{
}

void Profiler::Reset()
{
    m_line_to_execution_counts.clear();
    m_instruction_type_counts.fill(0);
    emit profilerReset();
}

void Profiler::setInstructionToLineMapping(const AssembledProgram &program)
{
    m_instruction_number_line_number_mapping = program.instruction_number_line_number_mapping;
} 

const std::array<size_t, static_cast<size_t>(InstructionType::NUMBER_OF_TYPES)> &
Profiler::getInstructionTypeCounts() const
{
    return m_instruction_type_counts;
}

int Profiler::getExecutionCountForLine(int lineNumber) const
{
    const auto it = m_line_to_execution_counts.find(lineNumber);
    if (it != m_line_to_execution_counts.end())
    {
        return it->second;
    }
    return 0;
}

void Profiler::setLineNumberToInstructionTypeMapping(const AssembledProgram &program)
{
    m_line_number_instruction_type_mapping.clear();
    for (const auto &[icUnit, _] : program.intermediate_code)
    {
        const auto lineNumber = icUnit.getLineNumber();
        const auto mnemonic = icUnit.getOpcode();
        if (!mnemonic.empty())
        {
            InstructionType instrType = instruction_set::getInstructionType(mnemonic);
            m_line_number_instruction_type_mapping[lineNumber] = instrType;
        }
    }
}

void Profiler::onVMStateChanged(const QMap<QString, QVariant> &vmState)
{
    const int sourceLine = resolveSourceLineFromState(vmState);
    if (sourceLine <= 0)
    {
        return;
    }

    ++m_line_to_execution_counts[sourceLine];
    ++m_instruction_type_counts[static_cast<size_t>(getInstructionTypeForLine(sourceLine))];
    //emit lineExecutionCountsUpdated(m_line_to_execution_counts);
    //emit lineExecutionCountIncrementSignal(sourceLine);
}

int Profiler::resolveSourceLineFromState(const QMap<QString, QVariant> &vmState) const
{
    // Primary path: use PC + assembler mapping for consistent results across VM variants.
    const QVariant pcVariant = vmState.value("ProgramCounter");
    bool ok = false;
    const qulonglong pc = pcVariant.toULongLong(&ok);
    if (ok)
    {
        const auto mappingIt = m_instruction_number_line_number_mapping.find(
            static_cast<unsigned int>(pc / 4));
        if (mappingIt != m_instruction_number_line_number_mapping.end())
        {
            return static_cast<int>(mappingIt->second);
        }
    }

    // Fallback for states where only EditorLines is populated.
    // it shoudl never come to this 
    //todo remove this piece of code and make sure all the vm states are populating the pc field 
    //correctly
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

InstructionType Profiler::getInstructionTypeForLine(int lineNumber) const
{
    const auto it = m_line_number_instruction_type_mapping.find(lineNumber);
    if (it != m_line_number_instruction_type_mapping.end())
    {
        return it->second;
    }
    return InstructionType::UNKNOWN;
}
