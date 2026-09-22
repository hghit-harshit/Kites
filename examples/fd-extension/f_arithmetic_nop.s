# F-Extension Arithmetic Test (NOP-separated, safe for ALL VMs)
# Tests: fadd.s, fsub.s, fmul.s, fdiv.s, fsqrt.s
# Expected results:
#   x10 = 15    (fadd.s:  10.0 + 5.0 = 15.0)
#   x11 = 5     (fsub.s:  10.0 - 5.0 = 5.0)
#   x12 = 50    (fmul.s:  10.0 * 5.0 = 50.0)
#   x13 = 2     (fdiv.s:  10.0 / 5.0 = 2.0)
#   x14 = 5     (fsqrt.s: sqrt(25.0) = 5.0)

# ── Setup integer values ──
addi x1, x0, 10
addi x2, x0, 5
addi x3, x0, 25
addi x0, x0, 0
addi x0, x0, 0

# ── Convert to float ──
fcvt.s.w f1, x1        # f1 = 10.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f2, x2        # f2 = 5.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f3, x3        # f3 = 25.0
addi x0, x0, 0
addi x0, x0, 0

# ── fadd.s ──
fadd.s f10, f1, f2     # f10 = 10.0 + 5.0 = 15.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x10, f10      # x10 = 15
addi x0, x0, 0
addi x0, x0, 0

# ── fsub.s ──
fsub.s f11, f1, f2     # f11 = 10.0 - 5.0 = 5.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x11, f11      # x11 = 5
addi x0, x0, 0
addi x0, x0, 0

# ── fmul.s ──
fmul.s f12, f1, f2     # f12 = 10.0 * 5.0 = 50.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x12, f12      # x12 = 50
addi x0, x0, 0
addi x0, x0, 0

# ── fdiv.s ──
fdiv.s f13, f1, f2     # f13 = 10.0 / 5.0 = 2.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x13, f13      # x13 = 2
addi x0, x0, 0
addi x0, x0, 0

# ── fsqrt.s ──
fsqrt.s f14, f3        # f14 = sqrt(25.0) = 5.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x14, f14      # x14 = 5
