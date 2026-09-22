addi x3, x0, 55
addi x4, x0, 400
sw   x3, 0(x4)       # memory[400] = 55
addi x1, x0, 200
add  x2, x1, x1      # x2 = 400 (address)
lw   x9, 0(x2)       # assume memory[400] = 55
