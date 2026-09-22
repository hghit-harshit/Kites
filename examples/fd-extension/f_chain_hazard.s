# F-Extension Chained Dependency Hazard Test (NO NOPs — for H_NF and H_F)
# Tests a long FPR dependency chain: every instruction depends on the previous.
# This exercises multi-cycle forwarding / stalling rigorously.
#
# Computation chain:
#   f1 = 20.0, f2 = 4.0, f3 = 3.0, f4 = 2.0, f5 = 1.0
#   f10 = f1 + f2             = 24.0
#   f11 = f10 - f3            = 21.0       (depends on f10)
#   f12 = f11 * f4            = 42.0       (depends on f11)
#   f13 = f12 / f4            = 21.0       (depends on f12, reuses f4)
#   f14 = f13 + f5            = 22.0       (depends on f13)
#
# Expected: x10 = 24, x11 = 21, x12 = 42, x13 = 21, x14 = 22

# ── Setup integer values ──
addi x1, x0, 20
addi x2, x0, 4
addi x3, x0, 3
addi x4, x0, 2
addi x5, x0, 1
addi x0, x0, 0
addi x0, x0, 0

# ── Convert to float (NOP-separated since these read GPRs, not FPRs) ──
fcvt.s.w f1, x1
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f2, x2
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f3, x3
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f4, x4
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f5, x5
addi x0, x0, 0
addi x0, x0, 0

# ── Chained FP operations (BACK-TO-BACK, no NOPs) ──
fadd.s f10, f1, f2          # f10 = 20.0 + 4.0 = 24.0
fsub.s f11, f10, f3         # f11 = 24.0 - 3.0 = 21.0   (RAW on f10)
fmul.s f12, f11, f4         # f12 = 21.0 * 2.0 = 42.0   (RAW on f11)
fdiv.s f13, f12, f4         # f13 = 42.0 / 2.0 = 21.0   (RAW on f12)
fadd.s f14, f13, f5         # f14 = 21.0 + 1.0 = 22.0   (RAW on f13)

# ── Drain and convert results ──
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x10, f10
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x11, f11
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x12, f12
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x13, f13
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x14, f14
