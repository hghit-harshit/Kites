addi x1, x0, 10
add  x2, x1, x1      # x2 = 20  (store data)
addi x3, x0, 200
sw   x2, 0(x3)       # memory[200] = 20
lw   x8, 0(x3)       # x8 = 20
