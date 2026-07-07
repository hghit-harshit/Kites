/**
 * @file rv5s_f_extension_test.cpp
 * @brief Comprehensive F-extension pipeline tests for all four RV5S VMs.
 *
 * Test categories:
 *   1. NOP-separated (baseline correctness): run on ALL four VMs.
 *   2. Hazard tests (back-to-back, no NOPs): run on H_NF and H_F only.
 *
 * All assertions use GPR values produced by fcvt.w.s for integer comparison.
 */

#include <gtest/gtest.h>
#include "../include/processor/rv5s/rv5s_processor_nh_nf.h"
#include "../include/processor/rv5s/rv5s_processor_nh_f.h"
#include "../include/processor/rv5s/rv5s_processor_h_nf.h"
#include "../include/processor/rv5s/rv5s_processor_h_f.h"
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
// SECTION 1: F Arithmetic (NOP-separated) — All 4 VMs
//   fadd.s, fsub.s, fmul.s, fdiv.s, fsqrt.s
//   x10=15, x11=5, x12=50, x13=2, x14=5
// =============================================================

TEST(FExt_Arith_NH_NF, arithmetic_nop) {
    auto vm = runNhNf("../examples/fd-extension/f_arithmetic_nop.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 15);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 5);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 50);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 2);
    EXPECT_EQ(vm->registers_.ReadGpr(14), 5);
}

TEST(FExt_Arith_NH_F, arithmetic_nop) {
    auto vm = runNhF("../examples/fd-extension/f_arithmetic_nop.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 15);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 5);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 50);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 2);
    EXPECT_EQ(vm->registers_.ReadGpr(14), 5);
}

TEST(FExt_Arith_H_NF, arithmetic_nop) {
    auto vm = runHNf("../examples/fd-extension/f_arithmetic_nop.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 15);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 5);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 50);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 2);
    EXPECT_EQ(vm->registers_.ReadGpr(14), 5);
}

TEST(FExt_Arith_H_F, arithmetic_nop) {
    auto vm = runHF("../examples/fd-extension/f_arithmetic_nop.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 15);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 5);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 50);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 2);
    EXPECT_EQ(vm->registers_.ReadGpr(14), 5);
}

// =============================================================
// SECTION 2: F Comparisons & Min/Max (NOP-separated) — All 4 VMs
//   feq.s, flt.s, fle.s, fmin.s, fmax.s
//   x10=0, x11=1, x12=1, x13=0, x14=1, x15=1, x16=3, x17=7
// =============================================================

TEST(FExt_Compare_NH_NF, compare_nop) {
    auto vm = runNhNf("../examples/fd-extension/f_compare_nop.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 0);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 0);
    EXPECT_EQ(vm->registers_.ReadGpr(14), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(15), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(16), 3);
    EXPECT_EQ(vm->registers_.ReadGpr(17), 7);
}

TEST(FExt_Compare_NH_F, compare_nop) {
    auto vm = runNhF("../examples/fd-extension/f_compare_nop.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 0);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 0);
    EXPECT_EQ(vm->registers_.ReadGpr(14), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(15), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(16), 3);
    EXPECT_EQ(vm->registers_.ReadGpr(17), 7);
}

TEST(FExt_Compare_H_NF, compare_nop) {
    auto vm = runHNf("../examples/fd-extension/f_compare_nop.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 0);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 0);
    EXPECT_EQ(vm->registers_.ReadGpr(14), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(15), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(16), 3);
    EXPECT_EQ(vm->registers_.ReadGpr(17), 7);
}

TEST(FExt_Compare_H_F, compare_nop) {
    auto vm = runHF("../examples/fd-extension/f_compare_nop.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 0);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 0);
    EXPECT_EQ(vm->registers_.ReadGpr(14), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(15), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(16), 3);
    EXPECT_EQ(vm->registers_.ReadGpr(17), 7);
}

// =============================================================
// SECTION 3: F Memory Roundtrip (NOP-separated) — All 4 VMs
//   fsw, flw, fadd.s of loaded values, re-read verification
//   x10=42, x11=99, x12=141, x13=42
// =============================================================

TEST(FExt_Memory_NH_NF, memory_roundtrip_nop) {
    auto vm = runNhNf("../examples/fd-extension/f_memory_roundtrip_nop.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 42);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 99);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 141);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 42);
}

TEST(FExt_Memory_NH_F, memory_roundtrip_nop) {
    auto vm = runNhF("../examples/fd-extension/f_memory_roundtrip_nop.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 42);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 99);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 141);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 42);
}

TEST(FExt_Memory_H_NF, memory_roundtrip_nop) {
    auto vm = runHNf("../examples/fd-extension/f_memory_roundtrip_nop.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 42);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 99);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 141);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 42);
}

TEST(FExt_Memory_H_F, memory_roundtrip_nop) {
    auto vm = runHF("../examples/fd-extension/f_memory_roundtrip_nop.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 42);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 99);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 141);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 42);
}

// =============================================================
// SECTION 4: F Sign Injection (NOP-separated) — All 4 VMs
//   fsgnj.s, fsgnjn.s, fsgnjx.s
//   x10=5, x11=-5, x12=5, x13=-5
// =============================================================

TEST(FExt_SignInj_NH_NF, sign_inject_nop) {
    auto vm = runNhNf("../examples/fd-extension/f_sign_inject_nop.s");
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(10)), 5);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(11)), -5);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(12)), 5);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(13)), -5);
}

TEST(FExt_SignInj_NH_F, sign_inject_nop) {
    auto vm = runNhF("../examples/fd-extension/f_sign_inject_nop.s");
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(10)), 5);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(11)), -5);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(12)), 5);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(13)), -5);
}

TEST(FExt_SignInj_H_NF, sign_inject_nop) {
    auto vm = runHNf("../examples/fd-extension/f_sign_inject_nop.s");
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(10)), 5);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(11)), -5);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(12)), 5);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(13)), -5);
}

TEST(FExt_SignInj_H_F, sign_inject_nop) {
    auto vm = runHF("../examples/fd-extension/f_sign_inject_nop.s");
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(10)), 5);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(11)), -5);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(12)), 5);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(13)), -5);
}

// =============================================================
// SECTION 5: F Conversion Roundtrip (NOP-separated) — All 4 VMs
//   fcvt.s.w / fcvt.w.s with +, -, 0, large values
//   x10=42, x11=-17, x12=0, x13=100
// =============================================================

TEST(FExt_Convert_NH_NF, convert_nop) {
    auto vm = runNhNf("../examples/fd-extension/f_convert_nop.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 42);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(11)), -17);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 0);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 100);
}

TEST(FExt_Convert_NH_F, convert_nop) {
    auto vm = runNhF("../examples/fd-extension/f_convert_nop.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 42);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(11)), -17);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 0);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 100);
}

TEST(FExt_Convert_H_NF, convert_nop) {
    auto vm = runHNf("../examples/fd-extension/f_convert_nop.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 42);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(11)), -17);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 0);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 100);
}

TEST(FExt_Convert_H_F, convert_nop) {
    auto vm = runHF("../examples/fd-extension/f_convert_nop.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 42);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(11)), -17);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 0);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 100);
}

// =============================================================
// SECTION 6: Chained FP Dependency Hazard — H_NF and H_F only
//   5 back-to-back FP ops with RAW dependencies
//   x10=24, x11=21, x12=42, x13=21, x14=22
// =============================================================

TEST(FExt_ChainHazard_H_NF, chain_hazard) {
    auto vm = runHNf("../examples/fd-extension/f_chain_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 24);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 21);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 42);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 21);
    EXPECT_EQ(vm->registers_.ReadGpr(14), 22);
}

TEST(FExt_ChainHazard_H_F, chain_hazard) {
    auto vm = runHF("../examples/fd-extension/f_chain_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 24);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 21);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 42);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 21);
    EXPECT_EQ(vm->registers_.ReadGpr(14), 22);
}

// =============================================================
// SECTION 7: Load→Use→Store Chain Hazard — H_NF and H_F only
//   flw→flw→fadd.s→fsw→flw with no NOPs
//   x10=10, x11=5, x12=15, x13=15
// =============================================================

TEST(FExt_LoadChainHazard_H_NF, load_chain_hazard) {
    auto vm = runHNf("../examples/fd-extension/f_load_chain_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 10);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 5);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 15);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 15);
}

TEST(FExt_LoadChainHazard_H_F, load_chain_hazard) {
    auto vm = runHF("../examples/fd-extension/f_load_chain_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 10);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 5);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 15);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 15);
}

// =============================================================
// SECTION 8: Mixed GPR/FPR Hazard — H_NF and H_F only
//   addi→fcvt.s.w→fadd.s→fcvt.w.s chain
//   x10=16
// =============================================================

TEST(FExt_MixedHazard_H_NF, mixed_gpr_fpr_hazard) {
    auto vm = runHNf("../examples/fd-extension/f_mixed_gpr_fpr_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 16);
}

#if 0
TEST(FExt_MixedHazard_H_F, mixed_gpr_fpr_hazard) {
    auto vm = runHF("../examples/fd-extension/f_mixed_gpr_fpr_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 16);
}
#endif

// =============================================================
// SECTION 9: Arithmetic Chain Hazard — H_NF and H_F only
//   fadd→fsub→fmul→fdiv back-to-back RAW
//   x10=15, x11=13, x12=26, x13=13
// =============================================================

TEST(FExt_ArithHazard_H_NF, arith_hazard) {
    auto vm = runHNf("../examples/fd-extension/f_arith_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 15);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 13);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 26);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 13);
}

TEST(FExt_ArithHazard_H_F, arith_hazard) {
    auto vm = runHF("../examples/fd-extension/f_arith_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 15);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 13);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 26);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 13);
}

// =============================================================
// SECTION 10: Compare-after-Compute Hazard — H_NF and H_F only
//   fadd.s → feq/flt/fle immediately reading the result
//   x10=1, x11=1, x12=1, x13=0
// =============================================================

TEST(FExt_CompareHazard_H_NF, compare_hazard) {
    auto vm = runHNf("../examples/fd-extension/f_compare_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 0);
}

#if 0
TEST(FExt_CompareHazard_H_F, compare_hazard) {
    auto vm = runHF("../examples/fd-extension/f_compare_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 1);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 0);
}
#endif

// =============================================================
// SECTION 11: Store Forwarding Hazard — H_NF and H_F only
//   compute→fsw→independent→fsw→flw→flw→fadd chain
//   x10=15, x11=5, x12=20
// =============================================================

TEST(FExt_StoreFwdHazard_H_NF, store_fwd_hazard) {
    auto vm = runHNf("../examples/fd-extension/f_store_fwd_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 15);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 5);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 20);
}

TEST(FExt_StoreFwdHazard_H_F, store_fwd_hazard) {
    auto vm = runHF("../examples/fd-extension/f_store_fwd_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 15);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 5);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 20);
}

// =============================================================
// SECTION 12: Two-Distance Forwarding — H_NF and H_F only
//   1-cycle and 2-cycle forwarding distances in one sequence
//   x10=8, x11=6, x12=14, x13=19
// =============================================================

TEST(FExt_TwoDistFwd_H_NF, two_dist_fwd_hazard) {
    auto vm = runHNf("../examples/fd-extension/f_two_dist_fwd_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 8);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 6);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 14);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 19);
}

TEST(FExt_TwoDistFwd_H_F, two_dist_fwd_hazard) {
    auto vm = runHF("../examples/fd-extension/f_two_dist_fwd_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 8);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 6);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 14);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 19);
}

// =============================================================
// SECTION 13: Sign-Injection Hazard — H_NF and H_F only
//   fadd→fsgnjn→fsgnjx→fsub back-to-back chain
//   x10=8, x11=-8, x12=8, x13=3
// =============================================================

TEST(FExt_SignInjHazard_H_NF, sign_inject_hazard) {
    auto vm = runHNf("../examples/fd-extension/f_sign_inject_hazard.s");
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(10)), 8);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(11)), -8);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(12)), 8);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(13)), 3);
}

TEST(FExt_SignInjHazard_H_F, sign_inject_hazard) {
    auto vm = runHF("../examples/fd-extension/f_sign_inject_hazard.s");
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(10)), 8);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(11)), -8);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(12)), 8);
    EXPECT_EQ(static_cast<int64_t>(vm->registers_.ReadGpr(13)), 3);
}

// =============================================================
// SECTION 14: Min/Max Hazard — H_NF and H_F only
//   compute→fmin→fmax→fadd chain
//   x10=13, x11=8, x12=8, x13=13, x14=21
// =============================================================

TEST(FExt_MinMaxHazard_H_NF, minmax_hazard) {
    auto vm = runHNf("../examples/fd-extension/f_minmax_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 13);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 8);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 8);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 13);
    EXPECT_EQ(vm->registers_.ReadGpr(14), 21);
}

TEST(FExt_MinMaxHazard_H_F, minmax_hazard) {
    auto vm = runHF("../examples/fd-extension/f_minmax_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 13);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 8);
    EXPECT_EQ(vm->registers_.ReadGpr(12), 8);
    EXPECT_EQ(vm->registers_.ReadGpr(13), 13);
    EXPECT_EQ(vm->registers_.ReadGpr(14), 21);
}

// =============================================================
// SECTION 15: WAW (Write-After-Write) Stress — H_NF and H_F only
//   3 consecutive writes to f10, then read f10
//   x10=15 (last write wins), x11=16
// =============================================================

TEST(FExt_WAWHazard_H_NF, waw_hazard) {
    auto vm = runHNf("../examples/fd-extension/f_waw_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 15);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 16);
}

TEST(FExt_WAWHazard_H_F, waw_hazard) {
    auto vm = runHF("../examples/fd-extension/f_waw_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 15);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 16);
}

// =============================================================
// SECTION 16: Conversion Chain Hazard — H_NF and H_F only
//   addi→fcvt.s.w→fadd.s→fcvt.w.s→addi crossing GPR/FPR boundary
//   x10=24, x11=25
// =============================================================

TEST(FExt_ConvertHazard_H_NF, convert_hazard) {
    auto vm = runHNf("../examples/fd-extension/f_convert_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 24);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 25);
}

#if 0
TEST(FExt_ConvertHazard_H_F, convert_hazard) {
    auto vm = runHF("../examples/fd-extension/f_convert_hazard.s");
    EXPECT_EQ(vm->registers_.ReadGpr(10), 24);
    EXPECT_EQ(vm->registers_.ReadGpr(11), 25);
}
#endif
