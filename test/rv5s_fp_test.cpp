/**
 * @file rv5s_fp_test.cpp
 * @brief Thorough floating-point pipeline tests for all four 5-stage VMs.
 *
 * Test strategy:
 *   - NOP-separated tests (fp_no_hazard, fp_fsw_flw_roundtrip): verify basic FP
 *     correctness on VMs that properly dispatch FP instructions (NH_NF, H_F).
 *   - Hazard tests (FR_FR, FL_FU, FR_FS, FL_FS): back-to-back FP dependencies
 *     with NO NOPs, exercising the HDU's FPR hazard detection.
 *     These only run on H_F (Hazard Detection + Forwarding).
 *
 * All assertions use GPR values produced by fcvt.w.s so we can use EXPECT_EQ.
 */

#include <gtest/gtest.h>
#include "../include/vm/rv5s_vms/rv5svm_nh_nf.h"
#include "../include/vm/rv5s_vms/rv5svm_nh_f.h"
#include "../include/vm/rv5s_vms/rv5svm_h_nf.h"
#include "../include/vm/rv5s_vms/rv5svm_h_f.h"
#include "../include/assembler/assembler.h"
#include "../include/utils.h"

// ─────────────────────────────────────────────────────────────
// Helper launchers — one per VM type
// ─────────────────────────────────────────────────────────────

static std::unique_ptr<RV5StageVM_NH_NF> runNhNf(const std::string& file)
{
    setupVmStateDirectory();
    auto vm = std::make_unique<RV5StageVM_NH_NF>();
    vm->LoadProgram(assemble(file));
    vm->DebugRun();
    return vm;
}

static std::unique_ptr<RV5StageVM_NH_F> runNhF(const std::string& file)
{
    setupVmStateDirectory();
    auto vm = std::make_unique<RV5StageVM_NH_F>();
    vm->LoadProgram(assemble(file));
    vm->DebugRun();
    return vm;
}

static std::unique_ptr<RV5StageVM_H_NF> runHNf(const std::string& file)
{
    setupVmStateDirectory();
    auto vm = std::make_unique<RV5StageVM_H_NF>();
    vm->LoadProgram(assemble(file));
    vm->DebugRun();
    return vm;
}

static std::unique_ptr<RV5StageVM_H_F> runHF(const std::string& file)
{
    setupVmStateDirectory();
    auto vm = std::make_unique<RV5StageVM_H_F>();
    vm->LoadProgram(assemble(file));
    vm->DebugRun();
    return vm;
}

// =============================================================
// SECTION 1: Baseline FP arithmetic (NOP-separated, no hazards)
//   Program: fp_no_hazard.s
//   6 + 2 = 8 → x10 = 8, feq → x11 = 1
// =============================================================

TEST(RV5S_FP_NH_NF, fp_baseline_no_hazard)
{
    auto vm = runNhNf("../examples/fp_no_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 8);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 1);
}

TEST(RV5S_FP_H_F, fp_baseline_no_hazard)
{
    auto vm = runHF("../examples/fp_no_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 8);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 1);
}

// =============================================================
// SECTION 2: FP memory roundtrip (NOP-separated)
//   Program: fp_fsw_flw_roundtrip.s
//   x10 = 3, x11 = 7, x12 = 10
// =============================================================

TEST(RV5S_FP_NH_NF, fp_memory_roundtrip)
{
    auto vm = runNhNf("../examples/fp_fsw_flw_roundtrip.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 3);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 7);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 10);
}

TEST(RV5S_FP_H_F, fp_memory_roundtrip)
{
    auto vm = runHF("../examples/fp_fsw_flw_roundtrip.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 3);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 7);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 10);
}

// =============================================================
// SECTION 3: FP R→R hazard (back-to-back fadd.s dependency)
//   Program: fp_FR_FR_hazard.s
//   (6+2=8) then (8+3=11) → x10 = 11
//   H_F should forward; H_NF should stall.
// =============================================================

TEST(RV5S_FP_H_F, fp_fr_fr_hazard)
{
    auto vm = runHF("../examples/fp_FR_FR_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 11);
}

TEST(RV5S_FP_H_NF, fp_fr_fr_hazard)
{
    auto vm = runHNf("../examples/fp_FR_FR_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 11);
}

// =============================================================
// SECTION 4: FP Load-Use hazard (flw → fadd.s)
//   Program: fp_FL_FU_hazard.s
//   6 + 7(loaded) = 13 → x10 = 13
//   H_F: 1-cycle stall (load-use), then forward.
//   H_NF: 2-cycle stall.
// =============================================================

TEST(RV5S_FP_H_F, fp_fl_fu_hazard)
{
    auto vm = runHF("../examples/fp_FL_FU_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 13);
}

TEST(RV5S_FP_H_NF, fp_fl_fu_hazard)
{
    auto vm = runHNf("../examples/fp_FL_FU_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 13);
}

// =============================================================
// SECTION 5: FP R→Store hazard (fadd.s → fsw)
//   Program: fp_FR_FS_hazard.s
//   (6+2=8, stored, loaded back) → x10 = 8
//   H_F: forwarding resolves (0 stalls).
//   H_NF: 2-cycle stall.
// =============================================================

TEST(RV5S_FP_H_F, fp_fr_fs_hazard)
{
    auto vm = runHF("../examples/fp_FR_FS_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 8);
}

TEST(RV5S_FP_H_NF, fp_fr_fs_hazard)
{
    auto vm = runHNf("../examples/fp_FR_FS_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 8);
}

// =============================================================
// SECTION 6: FP Load→Store hazard (flw → fsw)
//   Program: fp_FL_FS_hazard.s
//   9 stored, loaded, stored to new slot, loaded back → x10 = 9
//   H_F: 1-cycle stall (load-use), then forward store data.
//   H_NF: 2-cycle stall.
// =============================================================

TEST(RV5S_FP_H_F, fp_fl_fs_hazard)
{
    auto vm = runHF("../examples/fp_FL_FS_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 9);
}

TEST(RV5S_FP_H_NF, fp_fl_fs_hazard)
{
    auto vm = runHNf("../examples/fp_FL_FS_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 9);
}

// =============================================================
// SECTION 7: Existing NH_NF FP test (from f_extension_nh_nf_test.s)
//   Run on H_F to verify it also produces correct results.
//   x10 = 8, x11 = 1, x12 = 8
// =============================================================

TEST(RV5S_FP_H_F, f_extension_existing_test_on_h_f)
{
    auto vm = runHF("../examples/f_extension_nh_nf_test.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 8);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 8);
}
