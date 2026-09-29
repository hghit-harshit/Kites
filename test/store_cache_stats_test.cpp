#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <string>

#include "assembler/assembler.h"
#include "processor/cache/cache.h"
#include "processor/rv5s/rv5s_processor_h_f.h"
#include "processor/rv5s/rv5s_processor_h_nf.h"
#include "processor/rv5s/rv5s_processor_nh_f.h"
#include "processor/rv5s/rv5s_processor_nh_nf.h"
#include "processor/rvss/rvss_processor.h"
#include "utils/utils.h"

namespace
{

// Recording a store's old/new bytes for undo used to read them through the L1 cache,
// adding 8 extra accesses per `sw` (allocating lines and bumping replacement state).
// A single store into a cold cache must count as exactly one L1 access: a miss.
template <typename VmType>
void runSingleStoreCountsOneAccess()
{
    Kites::setupVmStateDirectory();

    auto vm = std::make_unique<VmType>();
    std::istringstream stream("addi x10, x0, 256\n"
                              "addi x11, x0, 42\n"
                              "nop\n"
                              "nop\n"
                              "nop\n"
                              "sw x11, 0(x10)\n");
    vm->LoadProgram(Kites::assemble(stream));
    vm->DebugRun();

    Kites::Cache *l1 = vm->memory_controller_.getL1Cache();
    EXPECT_EQ(l1->getMissCount(), 1u);
    EXPECT_EQ(l1->getHitCount(), 0u);
    EXPECT_EQ(vm->memory_controller_.peekByte(256), 42u);
}

} // namespace

TEST(StoreCacheStats, SingleCycle)
{
    runSingleStoreCountsOneAccess<Kites::RVSSProcessor>();
}

TEST(StoreCacheStats, HazardForwarding)
{
    runSingleStoreCountsOneAccess<Kites::RV5StageProcessorHF>();
}

TEST(StoreCacheStats, HazardNoForwarding)
{
    runSingleStoreCountsOneAccess<Kites::RV5StageProcessorHNF>();
}

TEST(StoreCacheStats, NoHazardForwarding)
{
    runSingleStoreCountsOneAccess<Kites::RV5StageProcessorNHF>();
}

TEST(StoreCacheStats, NoHazardNoForwarding)
{
    runSingleStoreCountsOneAccess<Kites::RV5StageProcessorNHNF>();
}
