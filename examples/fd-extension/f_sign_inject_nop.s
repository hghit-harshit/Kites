# F-Extension Sign-Injection Test (NOP-separated, safe for ALL VMs)
# Tests: fsgnj.s (fmv.s pseudo), fsgnjn.s (fneg.s pseudo), fsgnjx.s (fabs.s pseudo)
#
# Setup: f1 = 5.0, f2 = -3.0 (via fneg of 3.0)
#
# Expected:
#   x10 = 5      (fsgnj.s  f10, f1, f1 → copy f1 sign to f1 → 5.0, same as fmv.s)
#   x11 = -5     (fsgnjn.s f11, f1, f1 → negate sign of f1 → -5.0, same as fneg.s)
#   x12 = 5      (fsgnjx.s f12, f1, f1 → xor signs → positive → 5.0, same as fabs.s)
#   x13 = -5     (fsgnj.s  f13, f1, f2 → takes sign of f2(-) onto magnitude of f1 → -5.0)

addi x1, x0, 5
addi x2, x0, 3
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f1, x1        # f1 = 5.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f3, x2        # f3 = 3.0
addi x0, x0, 0
addi x0, x0, 0

fsgnjn.s f2, f3, f3    # f2 = fneg(f3) = -3.0
addi x0, x0, 0
addi x0, x0, 0

# ── fsgnj.s (copy: sign of src2 → result) — fmv.s pseudo ──
fsgnj.s f10, f1, f1    # f10 = +5.0 (sign of f1 is +)
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x10, f10      # x10 = 5
addi x0, x0, 0
addi x0, x0, 0

# ── fsgnjn.s (negate: inverted sign of src2 → result) — fneg.s pseudo ──
fsgnjn.s f11, f1, f1   # f11 = -5.0 (inverted sign of f1)
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x11, f11      # x11 = -5
addi x0, x0, 0
addi x0, x0, 0

# ── fsgnjx.s (xor: sign = sign_a XOR sign_b → result) — fabs.s pseudo ──
fsgnjx.s f12, f1, f1   # f12 = 5.0 (+ XOR + = +)
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x12, f12      # x12 = 5
addi x0, x0, 0
addi x0, x0, 0

# ── fsgnj.s with different sign source ──
fsgnj.s f13, f1, f2    # f13 = -5.0 (magnitude of f1, sign of f2 which is -)
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x13, f13      # x13 = -5
