addi x1, x0, 5        # x1 = 5
addi x2, x0, 10       # x2 = 10
addi x0, x0, 0        # NOP
addi x0, x0, 0        # NOP
add  x3, x1, x2       # x3 = 15
addi x0, x0, 0        # NOP
addi x0, x0, 0        # NOP
addi x4, x3, 5        # x4 = 20