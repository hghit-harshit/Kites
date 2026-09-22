addi x1, x0, 100
addi x2, x0, 3
sw  x2, 0(x1)       # memory[100] = 3
lw   x3, 0(x1)       # assume memory[100] = 3
beq  x2, x3, L1      # TAKEN
addi x7, x0, 0
jal  x0, END
L1:
addi x7, x0, 1 #x7 =1
END:
