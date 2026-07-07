#pragma once
#include "processor/processor_types.h"
#include "utils/to_index.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace Kites
{

enum class PipelineStage
{
    Current = 0,
    IF_ID,
    ID_EX,
    EX_MEM,
    MEM_WB,
    PipelineStageCount
};

struct PipelineState
{
    std::array<std::string, toIndex(PipelineStage::PipelineStageCount)> instructionTexts{};
    std::array<uint64_t, toIndex(PipelineStage::PipelineStageCount)> editorLineNumbers{};
    std::array<uint64_t, toIndex(PipelineStage::PipelineStageCount)> disassemblyLineNumbers{};
};

struct VMStatistics
{
    uint64_t cycleCount{};
    uint64_t instructionsRetiredCount{};
    uint64_t branchMispredictionCount{};
    uint64_t stallCycleCount{};
    double cpi{};
    double ipc{};
};

struct VMState
{
    VMType vmType;
    VMStatistics statistics{};
    PipelineState pipelineState{};
    void reset()
    {
        statistics = VMStatistics{};
        pipelineState = PipelineState{};
    }
};
} // namespace Kites
