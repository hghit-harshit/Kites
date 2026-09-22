# F-Extension Mixed GPR/FPR Hazard Test (NO NOPs — for H_NF and H_F)
# Tests: an integer instruction followed by FP instructions that read the
# same GPR (via fcvt.s.w), verifying that GPR hazard detection doesn't
# false-positive on FPR instructions that read GPRs, and vice-versa.
#
# Flow:
#   1. addi writes x5 = 8
#   2. Immediately fcvt.s.w reads x5 (GPR→FPR, hazard on GPR x5)
#   3. fadd.s uses the result in FPR space (hazard on FPR)
#   4. fcvt.w.s converts back (hazard on FPR again)
#
# Expected:
#   x10 = 16     (8.0 + 8.0 = 16.0)

addi x20, x0, 192
addi x0, x0, 0
addi x0, x0, 0

# ── GPR → FPR pipeline transition ──
addi x5, x0, 8          # x5 = 8
fcvt.s.w f1, x5         # f1 = 8.0 (GPR hazard: reads x5 from previous instruction)
addi x0, x0, 0
addi x0, x0, 0

# ── FPR → FPR hazard ──
fadd.s f2, f1, f1       # f2 = 16.0 (FPR hazard: reads f1)
addi x0, x0, 0
addi x0, x0, 0

# ── FPR → GPR transition ──
fcvt.w.s x10, f2        # x10 = 16 (FPR hazard: reads f2)
