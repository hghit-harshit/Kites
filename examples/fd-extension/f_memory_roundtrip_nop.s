# F-Extension Memory Roundtrip Test (NOP-separated, safe for ALL VMs)
# Tests: fsw, flw — store multiple FP values, load them back, verify integrity
# Expected results:
#   x10 = 42     (stored 42.0, loaded back)
#   x11 = 99     (stored 99.0, loaded back)
#   x12 = 141    (42.0 + 99.0 = 141.0, computed from loaded values)
#   x13 = 42     (verify re-reading slot 0 still correct)

# ── Setup base address for FP data ──
addi x20, x0, 192      # base address for FP storage

addi x1, x0, 42
addi x2, x0, 99
addi x0, x0, 0
addi x0, x0, 0

# ── Convert to float ──
fcvt.s.w f1, x1        # f1 = 42.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f2, x2        # f2 = 99.0
addi x0, x0, 0
addi x0, x0, 0

# ── Store to memory ──
fsw f1, 0(x20)          # mem[192] = 42.0
addi x0, x0, 0
addi x0, x0, 0

fsw f2, 4(x20)          # mem[196] = 99.0
addi x0, x0, 0
addi x0, x0, 0

# ── Load back from memory into different FPRs ──
flw f10, 0(x20)         # f10 = 42.0 (from mem[192])
addi x0, x0, 0
addi x0, x0, 0

flw f11, 4(x20)         # f11 = 99.0 (from mem[196])
addi x0, x0, 0
addi x0, x0, 0

# ── Add loaded values ──
fadd.s f12, f10, f11    # f12 = 42.0 + 99.0 = 141.0
addi x0, x0, 0
addi x0, x0, 0

# ── Re-read slot 0 to verify it's not corrupted ──
flw f13, 0(x20)         # f13 = 42.0 (should still be there)
addi x0, x0, 0
addi x0, x0, 0

# ── Convert all results to integer for assertion ──
fcvt.w.s x10, f10      # x10 = 42
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x11, f11      # x11 = 99
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x12, f12      # x12 = 141
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x13, f13      # x13 = 42
