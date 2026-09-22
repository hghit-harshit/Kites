# F-Extension Compare After Compute Hazard (NO NOPs — for H_NF and H_F)
# fadd.s produces a result, immediately followed by feq/flt/fle that read it.
#
# Setup:
#   f1 = 3.0, f2 = 4.0, f3 = 7.0
#
# Chain (NO NOPs):
#   f10 = f1 + f2                     = 7.0
#   x10 = feq.s(f10, f3)              = 1  (7.0 == 7.0, RAW on f10)
#   f11 = f1 + f1                     = 6.0
#   x11 = flt.s(f11, f3)              = 1  (6.0 < 7.0, RAW on f11)
#   f12 = f2 + f2                     = 8.0
#   x12 = fle.s(f3, f12)              = 1  (7.0 <= 8.0, no RAW — f12 is src2)
#   x13 = flt.s(f12, f3)              = 0  (8.0 < 7.0 → false, RAW on f12)
#
# Expected: x10 = 1, x11 = 1, x12 = 1, x13 = 0

addi x1, x0, 3
addi x2, x0, 4
addi x3, x0, 7
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

# ── Compute then compare (back-to-back) ──
fadd.s f10, f1, f2      # f10 = 7.0
feq.s x10, f10, f3      # x10 = 1 (RAW on f10)

fadd.s f11, f1, f1      # f11 = 6.0
flt.s x11, f11, f3      # x11 = 1 (RAW on f11)

fadd.s f12, f2, f2      # f12 = 8.0
fle.s x12, f3, f12      # x12 = 1 (RAW on f12 as src2)

flt.s x13, f12, f3      # x13 = 0 (f12 still available from 2 insns ago)
