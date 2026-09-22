# F-Extension WAW (Write-After-Write) Stress Test (NO NOPs — for H_NF and H_F)
# Multiple instructions write to the SAME FPR in sequence.
# Only the last write should be visible after the pipeline drains.
#
# Chain:
#   f10 = f1 + f2       = 8.0   (first write to f10)
#   f10 = f1 - f2       = 2.0   (second write to f10, overwrites)
#   f10 = f1 * f2       = 15.0  (third write to f10, overwrites)
#   f11 = f10 + f3      = 16.0  (reads latest f10 = 15.0, RAW hazard)
#
# Expected: x10 = 15 (last write wins), x11 = 16

addi x1, x0, 5
addi x2, x0, 3
addi x3, x0, 1
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f1, x1
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f2, x2
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f3, x3
addi x0, x0, 0
addi x0, x0, 0

# ── WAW chain on f10 ──
fadd.s f10, f1, f2      # f10 = 8.0
fsub.s f10, f1, f2      # f10 = 2.0 (WAW)
fmul.s f10, f1, f2      # f10 = 15.0 (WAW)
fadd.s f11, f10, f3     # f11 = 16.0 (must read f10 = 15.0, not 8.0 or 2.0)

addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x10, f10
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x11, f11
