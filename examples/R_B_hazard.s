addi x1, x0, 3
add  x2, x1, x1      # x2 = 6
beq  x2, x1, L1      # NOT taken (6 != 3)
addi x10, x0, 0
jal  x0, END
L1:
addi x10, x0, 1
END:
