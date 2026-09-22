addi x1, x0, 100
addi x2, x0, 7
sw   x2, 0(x1)       # memory[100] = 7
lw   x3, 0(x1)       
addi x3, x0, 300
sw   x2, 0(x3)
lw   x7, 0(x3)       # x7 = 7
