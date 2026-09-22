# 3x3 integer matrix multiplication to stress L1/L2 data caches.
# A base = 0x2000, B base = 0x2100, C base = 0x2200
# Data is seeded by the unit test (not here) to avoid warming L1.
# nops are added so that we can test it for every vm without worrying about pipeline hazards.
main:
    lui  t0, 0x2         # base A (0x2000)
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    addi t0, t0, 0x000

    lui  t1, 0x2         # base B (0x2100)
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    addi t1, t1, 0x100

    lui  t2, 0x2         # base C (0x2200)
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    addi t2, t2, 0x200

    li   t3, 0           # i = 0
    li   t4, 3           # N = 3

outer_i:
    li   t5, 0           # j = 0

outer_j:
    li   t6, 0           # sum = 0
    li   s0, 0           # k = 0

inner_k:
    # A[i][k]
    mul  s1, t3, t4
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    add  s1, s1, s0
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    slli s1, s1, 2
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    add  s2, t0, s1
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    lw   s3, 0(s2)

    # B[k][j]
    mul  s4, s0, t4
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    add  s4, s4, t5
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    slli s4, s4, 2
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    add  s5, t1, s4
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    lw   s6, 0(s5)
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0

    # sum += A[i][k] * B[k][j]
    mul  s7, s3, s6
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    add  t6, t6, s7

    addi s0, s0, 1
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    blt  s0, t4, inner_k

    # C[i][j] = sum
    mul  s1, t3, t4
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    add  s1, s1, t5
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    slli s1, s1, 2
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    add  s2, t2, s1
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    sw   t6, 0(s2)

    addi t5, t5, 1
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    blt  t5, t4, outer_j

    addi t3, t3, 1
    addi x0, x0, 0
    addi x0, x0, 0
    addi x0, x0, 0
    blt  t3, t4, outer_i

end:
    addi x0, x0, 0
