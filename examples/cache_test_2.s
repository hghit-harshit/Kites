main:
    addi t0, x0, 0x100     # base memory address for array
    addi t1, x0, 0           # i = 0
    addi t2, x0, 1024        # limit = 1024

# Initialize array
init_loop:
    sw t1, 0(t0)
    addi t0, t0, 4
    addi t1, t1, 1
    blt t1, t2, init_loop

# Sequential access test
    addi t0, x0, 0x00100
    addi t1, x0, 0
    addi t3, x0, 0           # sum = 0

seq_loop:
    lw t4, 0(t0)
    add t3, t3, t4
    addi t0, t0, 4
    addi t1, t1, 1
    blt t1, t2, seq_loop

# Strided access test (every 16th integer)
    addi t0, x0, 0x00100
    addi t1, x0, 0
    addi t5, x0, 16
    addi t6, x0, 0           # stride sum = 0

stride_loop:
    lw t4, 0(t0)
    add t6, t6, t4
    addi t0, t0, 64          # 16 × 4 bytes
    addi t1, t1, 16
    blt t1, t2, stride_loop

end:
    beq x0, x0, end