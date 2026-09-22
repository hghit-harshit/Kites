addi x10, x0, 128
addi x1, x0, 42
sw   x1, 0(x10)
lw   x2, 0(x10)
addi x3, x2, 1
sd   x3, 8(x10)
ld   x4, 8(x10)
add  x5, x4, x1
