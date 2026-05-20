#pragma once

#include <QMap>
#include <QObject>
#include <QVariant>
#include <map>
#include <string>


class ProfilerManager : public QObject
{
    Q_OBJECT

  public:
    explicit ProfilerManager(QObject *parent = nullptr);

    void Reset();
    void
    SetInstructionLineMapping(const std::map<unsigned int, unsigned int> &instructionToLineMapping);
    void SetInstructionInfo(const std::map<unsigned int, std::string> &instructionMnemonics);

  signals:
    void lineExecutionCountsUpdated(const std::map<int, int> &lineExecutionCounts);
    void profilerReset();

  public slots:
    void OnVMStateChanged(const QMap<QString, QVariant> &vmState);

  public:
    // Get instruction type for a specific line
    std::string GetInstructionTypeForLine(int lineNumber) const;
    // Get all instruction types mapped by line number
    const std::map<int, std::string> &GetLineInstructionTypes() const
    {
        return line_to_instruction_types_mapping_;
    }

  private:
    int ResolveSourceLineFromState(const QMap<QString, QVariant> &vmState) const;

    std::map<unsigned int, unsigned int> instruction_to_line_mapping_;
    std::map<unsigned int, std::string>
        instruction_index_to_mnemonics_;          // PC/instruction index -> mnemonic
    std::map<int, int> line_to_execution_counts_; // line number -> execution count
    std::map<int, std::string>
        line_to_instruction_types_mapping_; // line number -> instruction type
};