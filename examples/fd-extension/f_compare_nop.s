# F-Extension Comparison & Min/Max Test (NOP-separated, safe for ALL VMs)
# Tests: feq.s, flt.s, fle.s, fmin.s, fmax.s
# Expected results:
#   x10 = 0     (feq.s:  3.0 == 7.0 → false)
#   x11 = 1     (feq.s:  7.0 == 7.0 → true)
#   x12 = 1     (flt.s:  3.0 <  7.0 → true)
#   x13 = 0     (flt.s:  7.0 <  3.0 → false)
#   x14 = 1     (fle.s:  3.0 <= 7.0 → true)
#   x15 = 1     (fle.s:  7.0 <= 7.0 → true)
#   x16 = 3     (fmin.s: min(3.0, 7.0) = 3.0)
#   x17 = 7     (fmax.s: max(3.0, 7.0) = 7.0)

# ── Setup ──
addi x1, x0, 3
addi x2, x0, 7
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f1, x1        # f1 = 3.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f2, x2        # f2 = 7.0
addi x0, x0, 0
addi x0, x0, 0

# ── feq.s (not equal) ──
feq.s x10, f1, f2      # x10 = 0 (3.0 != 7.0)
addi x0, x0, 0
addi x0, x0, 0

# ── feq.s (equal) ──
feq.s x11, f2, f2      # x11 = 1 (7.0 == 7.0)
addi x0, x0, 0
addi x0, x0, 0

# ── flt.s (less than, true) ──
flt.s x12, f1, f2      # x12 = 1 (3.0 < 7.0)
addi x0, x0, 0
addi x0, x0, 0

# ── flt.s (less than, false) ──
flt.s x13, f2, f1      # x13 = 0 (7.0 < 3.0 → false)
addi x0, x0, 0
addi x0, x0, 0

# ── fle.s (less or equal, true strict) ──
fle.s x14, f1, f2      # x14 = 1 (3.0 <= 7.0)
addi x0, x0, 0
addi x0, x0, 0

# ── fle.s (less or equal, true equal) ──
fle.s x15, f2, f2      # x15 = 1 (7.0 <= 7.0)
addi x0, x0, 0
addi x0, x0, 0

# ── fmin.s ──
fmin.s f5, f1, f2      # f5 = min(3.0, 7.0) = 3.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x16, f5       # x16 = 3
addi x0, x0, 0
addi x0, x0, 0

# ── fmax.s ──
fmax.s f6, f1, f2      # f6 = max(3.0, 7.0) = 7.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x17, f6       # x17 = 7
