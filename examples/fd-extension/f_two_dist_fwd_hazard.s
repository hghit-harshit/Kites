# F-Extension Two-Distance Forwarding Test (NO NOPs — for H_NF and H_F)
# Tests that forwarding works at both 1-cycle and 2-cycle distances.
#
# Pattern:
#   Instruction A writes f10
#   Instruction B is independent (creates a 1-instruction gap)
#   Instruction C reads f10 (2-cycle distance — MEM/WB forwarding)
#
# Also tests 1-cycle distance (immediate RAW) for comparison.
#
# Flow:
#   f10 = f1 + f2         = 8.0   (producer A)
#   f11 = f3 + f3         = 6.0   (independent B)
#   f12 = f10 + f11       = 14.0  (reads f10 at 2-cycle dist, f11 at 1-cycle dist)
#   f13 = f12 + f1        = 19.0  (reads f12 at 1-cycle dist — EX/MEM forwarding)
#
# Expected: x10 = 8, x11 = 6, x12 = 14, x13 = 19

addi x1, x0, 5
addi x2, x0, 3
addi x3, x0, 3
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

# ── Mixed-distance forwarding ──
fadd.s f10, f1, f2      # f10 = 8.0 (producer A)
fadd.s f11, f3, f3      # f11 = 6.0 (independent B)
fadd.s f12, f10, f11    # f12 = 14.0 (f10 at 2-cycle, f11 at 1-cycle)
fadd.s f13, f12, f1     # f13 = 19.0 (f12 at 1-cycle, f1 is old)

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
