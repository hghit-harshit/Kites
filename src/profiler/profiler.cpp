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

// void Profiler::setInstructionTypeCounts(const AssembledProgram &program)
// {
//     m_instructionTypeCounts.fill(0);
//     for (const auto &[icUnit, _] : program.intermediate_code)
//     {
//         const auto mnemonic = icUnit.getOpcode();
//         if (!mnemonic.empty())
//         {
//             instr::InstructionType instrType = instr::getInstructionType(mnemonic);
//             m_instructionTypeCounts[toIndex(instrType)]++;
//         }
//     }
//     emit updateInstructionTypeCountsSignal(m_instructionTypeCounts);
// }

const InstructionTypeCounts &Profiler::getInstructionTypeCounts() const
{
    return m_instructionTypeCounts;
}

int Profiler::getLineExecutionCount(int lineNumber) const
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
    qDebug() << "Setting line number to instruction type mapping for program" ;
    for (const auto &[icUnit, _] : program.intermediate_code)
    {
        const auto lineNumber = icUnit.getLineNumber();
        const auto mnemonic = icUnit.getOpcode();
        if (!mnemonic.empty())
        {
            instr::InstructionType instrType = instr::getInstructionType(mnemonic);
            m_lineNumberToinstructionType[lineNumber] = instrType;
            m_instructionTypeCounts[toIndex(instrType)]++;
        }
    }
    emit updateLineInstructionTypeSignal(m_lineNumberToinstructionType);
}

void Profiler::processorClockedSlot(const ProcessorState &processorState)
{
    int executedLine = m_instructionNumberToLineNumber
    [static_cast<unsigned int>(processorState.lastExecutedPC / 4)];
    qDebug() << "Processor clocked. Last executed PC: " << processorState.lastExecutedPC
             << ", Instruction number: " << (processorState.lastExecutedPC / 4)
             << ", Mapped line number: " << executedLine;
    emit incrementLineExecutionCountSignal(executedLine);
}

// uint64_t Profiler::resolveExecutedLineFromState(const ProcessorState& processorState) const
// {
//     if (processorState.programCounters.empty())
//     {
//         return -1; // No program counters available
//     }

//     uint64_t programCounter = processorState.programCounters[0];
//     unsigned int instructionNumber = static_cast<unsigned int>(programCounter / 4);

//     const auto it = m_instructionNumberToLineNumber.find(instructionNumber);
//     if (it != m_instructionNumberToLineNumber.end())
//     {
//         return static_cast<int>(it->second);
//     }
//     return -1; // Not found
// }

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
