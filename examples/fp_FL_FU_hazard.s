# FP Load-Use hazard: flw writes f4, next fadd.s reads f4 immediately.
# H_NF: HDU stalls 2 cycles. H_F: HDU stalls 1 cycle (load-use).
# Expected: x10 = 13  (6 + 7 = 13, where 7 is loaded from memory)

addi x1, x0, 6
addi x5, x0, 7
addi x20, x0, 128
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f1, x1
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f2, x5
addi x0, x0, 0
addi x0, x0, 0

fsw f2, 0(x20)
addi x0, x0, 0
addi x0, x0, 0

flw f4, 0(x20)
fadd.s f3, f1, f4

addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x10, f3
