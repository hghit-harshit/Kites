# FP R-Store hazard: fadd.s writes f3, next fsw stores f3 immediately.
# H_NF: HDU stalls 2 cycles. H_F: forwarding resolves it (0 stalls).
# Expected: x10 = 8  (6 + 2 = 8, stored and loaded back)

addi x1, x0, 6
addi x2, x0, 2
addi x20, x0, 128
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f1, x1
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f2, x2
addi x0, x0, 0
addi x0, x0, 0

fadd.s f3, f1, f2
fsw f3, 0(x20)

addi x0, x0, 0
addi x0, x0, 0

flw f4, 0(x20)
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x10, f4
