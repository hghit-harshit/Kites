addi x1, x0, 100
addi x2, x0, 20
sw   x2, 0(x1)       # memory[100] = 20
lw   x3, 0(x1)      # assume memory[100] = 20
add  x6, x3, x1     # x6 = 20 + 100 = 120
