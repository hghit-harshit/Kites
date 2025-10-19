lui x3, 0x10000

ld x10, 0(x3)          # Load the first double from memory into x10
ld x11, 8(x3)          # Load the second double from memory into x11

gcd_loop:
    beq x11, x0, end_gcd      # if x11 == 0, done
    rem x12, x10, x11         # x12 = x10 % x11
    mv x10, x11               # x10 = x11
    mv x11, x12               # x11 = x12
    jal x0, gcd_loop          # repeat

end_gcd:
    nop
    nop
    # Result is in x10