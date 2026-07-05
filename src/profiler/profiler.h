#pragma once

#include <QMap>
#include <QObject>
#include <QVariant>
#include <map>
#include <string>
#include "common/assembled_program.h"
#include "common/instruction_types.h"

namespace Kites
{

using InstructionTypeCounts = std::array<size_t,static_cast<size_t>
                            (instruction_set::InstructionType::INSTRUCTION_TYPE_COUNT)>;
class Profiler : public QObject
{
    Q_OBJECT
  
  public:
    explicit Profiler(QObject *parent = nullptr);

    void Reset();
    // void SetInstructionLineMapping(const std::map<unsigned int, unsigned int>  
    //                                 &instructionToLineMapping);
    // void SetInstructionInfo(const std::map<unsigned int, std::string> &instructionMnemonics);

    //void setInstructionIndexToMnemonicMapping(const AssembledProgram &program);
    void setInstructionToLineMapping(const AssembledProgram &program);
     
    void setLineNumberToInstructionTypeMapping(const AssembledProgram &program);
    const InstructionTypeCounts& getInstructionTypeCounts() const;

    int getExecutionCountForLine(int lineNumber)const;
  signals:
    //void lineExecutionCountsUpdated(const std::map<int, int> &lineExecutionCounts);
    // Signal emitted when an instruction line's execution count is incremented by 1
    //void lineExecutionCountIncrementSignal(int lineNumber); 
    void profilerReset();

  public slots:
    void onVMStateChanged(const QMap<QString, QVariant> &vmState);

  public:
    // Get instruction type for a specific line
    instruction_set::InstructionType getInstructionTypeForLine(int lineNumber) const;
    // Get all instruction types mapped by line number
    const std::map<int, instruction_set::InstructionType> getLineInstructionTypes() const
    {
      return m_line_number_instruction_type_mapping;
    }

  private:
    int resolveSourceLineFromState(const QMap<QString, QVariant> &vmState) const;

    // std::map<unsigned int, unsigned int> m_instruction_to_line_mapping{}; 
    // std::map<unsigned int, std::string>  m_instruction_index_to_mnemonics_mapping{};   
    std::map<unsigned int, unsigned int> m_instruction_number_line_number_mapping{};       
    std::map<int, int>                   m_line_to_execution_counts{}; 
    std::map<int, instruction_set::InstructionType>       m_line_number_instruction_type_mapping{};

    InstructionTypeCounts m_instructionTypeCounts{};
    
};
}//namespace Kites
