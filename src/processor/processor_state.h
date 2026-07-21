#pragma once
#include "processor/processor_types.h"
#include "utils/to_index.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace Kites
{
enum class PipelineStage
{
    IF,
    ID,
    EX,
    MEM,
    WB,
    PipelineStageCount
};

inline std::string pipelineStageToString(PipelineStage stage)
{
    switch (stage)
    {
    case PipelineStage::IF:
        return "IF";
    case PipelineStage::ID:
        return "ID";
    case PipelineStage::EX:
        return "EX";
    case PipelineStage::MEM:
        return "MEM";
    case PipelineStage::WB:
        return "WB";
    default:
        return "";
    }
}
// struct PipelineData
// {
//     std::array<std::string, toIndex(PipelineStage::PipelineStageCount)> instructionTexts{};
//     std::array<uint64_t, toIndex(PipelineStage::PipelineStageCount)> sourceLineNumbers{};
//     std::array<uint64_t, toIndex(PipelineStage::PipelineStageCount)> disassemblyLineNumbers{};
// };

// struct ProcessorStatistics
// {
//     uint64_t cycleCount{};
//     uint64_t instructionsRetiredCount{};
//     uint64_t branchMispredictionCount{};
//     uint64_t stallCycleCount{};
//     double cpi{};
//     double ipc{};
// };

// struct SingleStageProcessorState
// {
//     uint64_t programCounter{};
// };

// struct FiveStageProcessorState
// {
//     std::array<uint64_t, toIndex(PipelineStage::PipelineStageCount)> programCounters{};
//     bool isStalled{};
// };

// using ProcessorState = std::variant<SingleStageProcessorState, FiveStageProcessorState>;

struct ProcessorState
{
    std::array<uint64_t, toIndex(PipelineStage::PipelineStageCount)> programCounters{};
    void reset()
    {
        programCounters.fill(0);
    }
    //for further extension we can add more fields here
    // tho beware if this struct is too big we loose on performance
};

} // namespace Kites
