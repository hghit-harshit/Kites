# Custom LFU policy access pattern demo
# Recommended cache config to mirror the unit test behavior:
#   sets = 1, block_size = 1 word, ways = 2
#   replacement policy = Custom (load lfu.lua)
#
# Address mapping in this program:
#   A = 0x1000
#   B = 0x1004
#   C = 0x1008
#
# Intended LFU behavior:
# 1) Fill cache with A and B
# 2) Make A more frequent than B
# 3) Access C to force eviction -> LFU should evict B
# 4) Re-read A (expected hit), then B (expected miss)

main:
    li   t0, 0x1000          # base address

    # Seed memory with distinct values so loads are easy to inspect.
    li   t1, 111
    sw   t1, 0(t0)           # A
    li   t1, 222
    sw   t1, 4(t0)           # B
    li   t1, 333
    sw   t1, 8(t0)           # C

    # Fill 2-way set with A and B
    lw   t2, 0(t0)           # A
    lw   t3, 4(t0)           # B

    # Increase A frequency much more than B
    lw   t2, 0(t0)           # A
    lw   t2, 0(t0)           # A
    lw   t2, 0(t0)           # A
    lw   t3, 4(t0)           # B

    # Force an eviction by bringing C
    lw   t4, 8(t0)           # C, LFU should evict B

    # Under LFU custom policy: A should still be cached, B should miss
    lw   t5, 0(t0)           # expected hit
    lw   t6, 4(t0)           # expected miss (B was evicted)

end:
    beq  x0, x0, end
