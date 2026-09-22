# F-Extension Min/Max Hazard (NO NOPs — for H_NF and H_F)
# Compute values, immediately pass to fmin.s / fmax.s.
#
# Setup:
#   f1 = 4.0, f2 = 9.0, f3 = 1.0
#
# Chain (NO NOPs):
#   f10 = fadd.s(f1, f2)      = 13.0
#   f11 = fsub.s(f2, f3)      = 8.0
#   f12 = fmin.s(f10, f11)    = 8.0   (RAW on both f10 at 2-dist, f11 at 1-dist)
#   f13 = fmax.s(f10, f11)    = 13.0  (f10 at 3-dist, f11 at 2-dist — from register file)
#   f14 = fadd.s(f12, f13)    = 21.0  (RAW on both f12 at 2-dist, f13 at 1-dist)
#
# Expected: x10 = 13, x11 = 8, x12 = 8, x13 = 13, x14 = 21

addi x1, x0, 4
addi x2, x0, 9
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

# ── Min/max with hazard chain ──
fadd.s f10, f1, f2      # f10 = 13.0
fsub.s f11, f2, f3      # f11 = 8.0
fmin.s f12, f10, f11    # f12 = 8.0  (RAW on f10 and f11)
fmax.s f13, f10, f11    # f13 = 13.0 (f10 from RF, f11 at 2-dist)
fadd.s f14, f12, f13    # f14 = 21.0 (RAW on f12 and f13)

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
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x14, f14
