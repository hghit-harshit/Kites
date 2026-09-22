# FP R-R hazard: back-to-back FPR dependency (no NOPs)
# fadd.s writes f3, next fadd.s reads f3 immediately.
# H_NF: HDU stalls 2 cycles. H_F: forwarding resolves it (0 stalls).
# Expected: x10 = 11  (6 + 2 = 8, then 8 + 3 = 11)

addi x1, x0, 6
addi x2, x0, 2
addi x3, x0, 3
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f1, x1
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f2, x2
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f5, x3
addi x0, x0, 0
addi x0, x0, 0

fadd.s f3, f1, f2
fadd.s f4, f3, f5

addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x10, f4
