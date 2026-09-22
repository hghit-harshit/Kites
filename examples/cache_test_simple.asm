main:
    addi t0, x0, 100      # memory address

    addi t1, x0, 5
    sw t1, 0(t0)          # store 5 at address 100

    lw t2, 0(t0)          # miss: first access loads cache line
    lw t3, 0(t0)          # hit: same address again
    lw t4, 0(t0)          # hit: same address again
    lw t5, 0(t0)          # hit: same address again

end:
    beq x0, x0, end