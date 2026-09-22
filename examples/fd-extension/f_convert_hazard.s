# F-Extension Convert Chain Hazard (NO NOPs — for H_NF and H_F)
# Tests back-to-back conversion operations creating RAW hazards.
#
# Flow:
#   addi x5, x0, 12      (GPR write)
#   fcvt.s.w f1, x5      (GPR→FPR, hazard on x5)
#   fadd.s f2, f1, f1    (FPR hazard on f1)
#   fcvt.w.s x10, f2     (FPR→GPR, hazard on f2)
#   addi x11, x10, 1     (GPR hazard on x10)
#
# Expected: x10 = 24 (12+12), x11 = 25

addi x20, x0, 192
addi x0, x0, 0
addi x0, x0, 0

addi x5, x0, 12
fcvt.s.w f1, x5          # f1 = 12.0 (GPR hazard on x5)
fadd.s f2, f1, f1        # f2 = 24.0 (FPR hazard on f1)

addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x10, f2         # x10 = 24 (FPR hazard on f2)
addi x11, x10, 1         # x11 = 25 (GPR hazard on x10)

addi x0, x0, 0
addi x0, x0, 0
