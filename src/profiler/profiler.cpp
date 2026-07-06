#include "profiler/profiler.h"
#include "common/instructions.h"

namespace Kites
{

namespace instr = instruction_set;

Profiler::Profiler(QObject *parent) : QObject(parent)
{
}

void Profiler::Reset()
{
    m_lineNumberToExecutionCounts.clear();
    m_instructionTypeCounts.fill(0);
    emit profilerReset();
}

void Profiler::setInstructionToLineMapping(const AssembledProgram &program)
{
    m_instructionNumberToLineNumber = program.instruction_number_line_number_mapping;
} 

const InstructionTypeCounts& Profiler::getInstructionTypeCounts() const
{
    return m_instructionTypeCounts;
}

int Profiler::getExecutionCountForLine(int lineNumber) const
{
    const auto it = m_lineNumberToExecutionCounts.find(lineNumber);
    if (it != m_lineNumberToExecutionCounts.end())
    {
        return it->second;
    }
    return 0;
}

void Profiler::setLineNumberToInstructionTypeMapping(const AssembledProgram &program)
{
    m_lineNumberToinstructionType.clear();
    for (const auto &[icUnit, _] : program.intermediate_code)
    {
        const auto lineNumber = icUnit.getLineNumber();
        const auto mnemonic = icUnit.getOpcode();
        if (!mnemonic.empty())
        {
            instr::InstructionType instrType = instr::getInstructionType(mnemonic);
            m_lineNumberToinstructionType[lineNumber] = instrType;
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

    ++m_lineNumberToExecutionCounts[sourceLine];
    ++m_instructionTypeCounts[toIndex(getInstructionTypeForLine(sourceLine))];
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
        const auto mappingIt = m_instructionNumberToLineNumber.find(
            static_cast<unsigned int>(pc / 4));
        if (mappingIt != m_instructionNumberToLineNumber.end())
        {
            return static_cast<int>(mappingIt->second);
        }
    }

    return -1;
}

instr::InstructionType Profiler::getInstructionTypeForLine(int lineNumber) const
{
    const auto it = m_lineNumberToinstructionType.find(lineNumber);
    if (it != m_lineNumberToinstructionType.end())
    {
        return it->second;
    }
    return instr::InstructionType::UNKNOWN;
}
}//namespace Kites
