# F-Extension Sign-Inject Hazard (NO NOPs — for H_NF and H_F)
# Back-to-back sign injection reading freshly produced values.
#
# Setup:
#   f1 = 5.0, f2 = 3.0
#
# Chain (NO NOPs):
#   f10 = fadd.s(f1, f2)            = 8.0
#   f11 = fsgnjn.s(f10, f10)        = -8.0  (fneg of f10, RAW on f10)
#   f12 = fsgnjx.s(f11, f11)        = 8.0   (fabs of f11, RAW on f11)
#   f13 = fsub.s(f12, f1)           = 3.0   (RAW on f12)
#
# Expected: x10 = 8, x11 = -8, x12 = 8, x13 = 3

addi x1, x0, 5
addi x2, x0, 3
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f1, x1
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f2, x2
addi x0, x0, 0
addi x0, x0, 0

# ── Sign injection chain (back-to-back) ──
fadd.s f10, f1, f2          # f10 = 8.0
fsgnjn.s f11, f10, f10      # f11 = -8.0 (fneg, RAW on f10)
fsgnjx.s f12, f11, f11      # f12 = 8.0  (fabs, RAW on f11)
fsub.s f13, f12, f1         # f13 = 3.0  (RAW on f12)

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
