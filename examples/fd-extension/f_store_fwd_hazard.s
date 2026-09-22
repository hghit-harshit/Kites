# F-Extension Store Forwarding Hazard (NO NOPs — for H_NF and H_F)
# Compute a result, immediately store it, then load it back.
# Tests compute→store RAW (store data forwarding) and store→load hazard.
#
# Flow:
#   f10 = f1 + f2        = 15.0          (fadd)
#   fsw f10, 0(x20)                      (RAW: store reads f10 from prev insn)
#   f11 = f1 - f2        = 5.0           (independent — no hazard)
#   fsw f11, 4(x20)                      (RAW: store reads f11 from prev insn)
#   flw f20, 0(x20)                      (read back 15.0)
#   flw f21, 4(x20)                      (read back 5.0)
#   fadd.s f22, f20, f21 = 20.0          (load-use on f20 AND f21)
#
# Expected: x10 = 15, x11 = 5, x12 = 20

addi x1, x0, 10
addi x2, x0, 5
addi x20, x0, 192
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f1, x1
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f2, x2
addi x0, x0, 0
addi x0, x0, 0

# ── Compute → Store → Load → Use chain (NO NOPs) ──
fadd.s f10, f1, f2      # f10 = 15.0
fsw f10, 0(x20)          # store f10 (RAW: reads f10 from prev cycle)
fsub.s f11, f1, f2      # f11 = 5.0 (independent of store)
fsw f11, 4(x20)          # store f11 (RAW: reads f11 from prev cycle)
flw f20, 0(x20)          # f20 = 15.0
flw f21, 4(x20)          # f21 = 5.0
fadd.s f22, f20, f21     # f22 = 20.0 (load-use on f20 and f21)

addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x10, f20
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x11, f21
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x12, f22
