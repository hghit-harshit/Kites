# F extension test program for RV5Stage NH_NF mode
# Uses explicit NOP spacing (2 NOPs) between producer-consumer dependencies.

addi x1, x0, 6
addi x2, x0, 2
addi x0, x0, 0
fcvt.s.w f1, x1
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f2, x2
addi x0, x0, 0
addi x0, x0, 0
addi x0, x0, 0
# f3 = 6.0 + 2.0 = 8.0
fadd.s f3, f1, f2
addi x0, x0, 0
addi x0, x0, 0

# Convert back to integer for easy assertion
fcvt.w.s x10, f3
addi x0, x0, 0
addi x0, x0, 0

# Self equality check should produce 1 in GPR
feq.s x11, f3, f3
addi x0, x0, 0
addi x0, x0, 0

# Store/load roundtrip through memory
addi x20, x0, 128
addi x0, x0, 0
addi x0, x0, 0
fsw f3, 0(x20)
addi x0, x0, 0
addi x0, x0, 0

flw f4, 0(x20)
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x12, f4
