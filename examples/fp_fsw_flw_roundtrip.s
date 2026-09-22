# FP memory roundtrip: store and load multiple FP values, with back-to-back hazards.
# Tests FP store/load correctness and FPR hazard detection across the pipeline.
# Expected: x10 = 3, x11 = 7, x12 = 10

addi x1, x0, 3
addi x2, x0, 7
addi x20, x0, 192
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f1, x1
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f2, x2
addi x0, x0, 0
addi x0, x0, 0

fsw f1, 0(x20)
addi x0, x0, 0
addi x0, x0, 0

fsw f2, 4(x20)
addi x0, x0, 0
addi x0, x0, 0

flw f3, 0(x20)
addi x0, x0, 0
addi x0, x0, 0

flw f4, 4(x20)
addi x0, x0, 0
addi x0, x0, 0

fadd.s f5, f3, f4
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x10, f3
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x11, f4
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x12, f5
