# F-Extension Conversion Test (NOP-separated, safe for ALL VMs)
# Tests: fcvt.s.w, fcvt.w.s roundtrip with positive, negative, and zero values
#
# Expected:
#   x10 = 42     (positive roundtrip: 42 → 42.0 → 42)
#   x11 = -17    (negative roundtrip: -17 → -17.0 → -17)
#   x12 = 0      (zero roundtrip: 0 → 0.0 → 0)
#   x13 = 100    (large value: 100 → 100.0 → 100)

# ── Positive value ──
addi x1, x0, 42
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f1, x1        # f1 = 42.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x10, f1       # x10 = 42
addi x0, x0, 0
addi x0, x0, 0

# ── Negative value ──
addi x2, x0, -17
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f2, x2        # f2 = -17.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x11, f2       # x11 = -17
addi x0, x0, 0
addi x0, x0, 0

# ── Zero ──
addi x3, x0, 0
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f3, x3        # f3 = 0.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x12, f3       # x12 = 0
addi x0, x0, 0
addi x0, x0, 0

# ── Larger value ──
addi x4, x0, 100
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f4, x4        # f4 = 100.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x13, f4       # x13 = 100
