li   x1, 0x1000
li   x2, 0x9000      # same cache index as 0x1000
li   x10, 111
li   x11, 222

sw   x10, 0(x1)      # write 111 to memory at 0x1000
sw   x11, 0(x2)      # write 222 to memory at 0x9000

lw   x3, 0(x1)       # MISS: loads line for 0x1000 into cache (x3 = 111)
lw   x4, 0(x2)       # MISS: replaces that line with line for 0x9000 (x4 = 222)
lw   x5, 0(x1)       # MISS again: replaces back with line for 0x1000 (x5 = 111)