#pragma once

#include <QMap>
#include <QObject>
#include <QVariant>
#include <map>
#include <string>
#include "common/assembled_program.h"
#include "common/instruction_types.h"
#include "utils/to_index.h"
#include "processor/processor_state.h"

namespace Kites
{

using InstructionTypeCounts = std::array<size_t, 
                            toIndex(instruction_set::InstructionType::INSTRUCTION_TYPE_COUNT)>;
                            
class Profiler : public QObject
{
    Q_OBJECT
  
public:
    explicit Profiler(QObject *parent = nullptr);

    void Reset();
    void setInstructionToLineMapping(const AssembledProgram &program);
     
    void setLineNumberToInstructionTypeMapping(const AssembledProgram &program);
    //void setInstructionTypeCounts(const AssembledProgram &program);

    const InstructionTypeCounts& getInstructionTypeCounts() const;

    int getLineExecutionCount(int lineNumber)const;

public slots:
    void processorClockedSlot(const ProcessorState& processorState);

public:
    // Get instruction type for a specific line
    // uint64_t resolveExecutedLineFromState(const ProcessorState& processorState) const;
    instruction_set::InstructionType getInstructionTypeForLine(int lineNumber) const;
    // Get all instruction types mapped by line number
    const std::map<int, instruction_set::InstructionType> getLineInstructionTypes() const
    {
      return m_lineNumberToinstructionType;
    }

private:
   
    std::map<unsigned int, unsigned int> m_instructionNumberToLineNumber{};       
    std::map<int, int>                   m_lineNumberToExecutionCounts{}; 
    std::map<int, instruction_set::InstructionType>       m_lineNumberToinstructionType{};

    InstructionTypeCounts m_instructionTypeCounts{};
signals:
    void profilerReset();
    void incrementLineExecutionCountSignal(int lineNumber);
    void updateLineInstructionTypeSignal(std::map<int, instruction_set::InstructionType> lineInstructionTypes = {});
    void updateInstructionTypeCountsSignal();
};
}//namespace Kites
