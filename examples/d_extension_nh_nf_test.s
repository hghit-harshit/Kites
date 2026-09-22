# D extension test program for RV5Stage NH_NF mode
# Uses explicit NOP spacing (2 NOPs) between producer-consumer dependencies.

addi x1, x0, 9
addi x2, x0, 4

fcvt.d.w f1, x1
addi x0, x0, 0
addi x0, x0, 0

fcvt.d.w f2, x2
addi x0, x0, 0
addi x0, x0, 0

# f3 = 9.0 + 4.0 = 13.0
fadd.d f3, f1, f2
addi x0, x0, 0
addi x0, x0, 0

# Convert back to integer for easy assertion
fcvt.w.d x10, f3
addi x0, x0, 0
addi x0, x0, 0

# Self equality check should produce 1 in GPR
feq.d x11, f3, f3
addi x0, x0, 0
addi x0, x0, 0

# Store/load roundtrip through memory
addi x20, x0, 160
addi x0, x0, 0
addi x0, x0, 0
fsd f3, 0(x20)
addi x0, x0, 0
addi x0, x0, 0

fld f4, 0(x20)
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.d x12, f4
