# D extension memory-focused test for RV5Stage NH_NF mode
# Validates fsd/fld roundtrip across multiple slots and overwrite behavior.

addi x1, x0, 5
addi x2, x0, 17

fcvt.d.w f1, x1
addi x0, x0, 0
addi x0, x0, 0

fcvt.d.w f2, x2
addi x0, x0, 0
addi x0, x0, 0

addi x20, x0, 224
addi x0, x0, 0
addi x0, x0, 0

fsd f1, 0(x20)
addi x0, x0, 0
addi x0, x0, 0

fsd f2, 8(x20)
addi x0, x0, 0
addi x0, x0, 0

fld f3, 0(x20)
addi x0, x0, 0
addi x0, x0, 0

fld f4, 8(x20)
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.d x10, f3
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.d x11, f4
addi x0, x0, 0
addi x0, x0, 0

feq.d x12, f1, f3
addi x0, x0, 0
addi x0, x0, 0

feq.d x13, f2, f4
addi x0, x0, 0
addi x0, x0, 0

# Overwrite first slot and verify it reads the new value.
fsd f2, 0(x20)
addi x0, x0, 0
addi x0, x0, 0

fld f5, 0(x20)
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.d x14, f5
