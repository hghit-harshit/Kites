# FP Load-Store hazard: flw writes f3, next fsw stores f3 immediately.
# H_NF: HDU stalls 2 cycles. H_F: HDU stalls 1 cycle (load-use), then forwards.
# Expected: x10 = 9  (9 is stored to slot A, loaded, stored to slot B, loaded back)

addi x5, x0, 9
addi x20, x0, 128
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f1, x5
addi x0, x0, 0
addi x0, x0, 0

fsw f1, 0(x20)
addi x0, x0, 0
addi x0, x0, 0

flw f3, 0(x20)
fsw f3, 4(x20)

addi x0, x0, 0
addi x0, x0, 0

flw f4, 4(x20)
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x10, f4
