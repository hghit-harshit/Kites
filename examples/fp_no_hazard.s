# FP baseline test - no data hazards (NOPs separate all dependencies)
# Works correctly on ALL four VMs.
# Expected: x10 = 8, x11 = 1

addi x1, x0, 6
addi x2, x0, 2
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f1, x1
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f2, x2
addi x0, x0, 0
addi x0, x0, 0

fadd.s f3, f1, f2
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x10, f3
addi x0, x0, 0
addi x0, x0, 0

feq.s x11, f3, f3
