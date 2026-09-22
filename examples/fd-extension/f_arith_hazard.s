# F-Extension Arithmetic Hazard (NO NOPs — for H_NF and H_F)
# Back-to-back fadd.s, fsub.s, fmul.s, fdiv.s with RAW dependencies.
# Each operation's result feeds directly into the next.
#
# Setup (NOP-separated since fcvt reads GPRs):
#   f1 = 10.0, f2 = 5.0, f3 = 2.0
#
# Chain (NO NOPs):
#   f10 = f1 + f2       = 15.0    (fadd)
#   f11 = f10 - f3      = 13.0    (fsub, RAW on f10)
#   f12 = f11 * f3      = 26.0    (fmul, RAW on f11)
#   f13 = f12 / f3      = 13.0    (fdiv, RAW on f12)
#
# Expected: x10 = 15, x11 = 13, x12 = 26, x13 = 13

addi x1, x0, 10
addi x2, x0, 5
addi x3, x0, 2
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

# ── Back-to-back chain ──
fadd.s f10, f1, f2
fsub.s f11, f10, f3
fmul.s f12, f11, f3
fdiv.s f13, f12, f3

# ── Drain and readout ──
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x10, f10
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x11, f11
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x12, f12
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x13, f13
