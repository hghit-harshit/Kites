# F-Extension Load→Use→Store Chain Hazard (NO NOPs — for H_NF and H_F)
# Tests: flw → fadd.s → fsw → flw → verify
# This exercises load-use hazard, then compute-to-store forwarding,
# then another load to verify the stored result.
#
# Flow:
#   1. Store 10.0 and 5.0 to memory
#   2. Load them back (flw) into different FPRs
#   3. Immediately add (fadd.s) — load-use hazard on both operands
#   4. Immediately store result (fsw) — compute-to-store hazard
#   5. Load the result back and verify
#
# Expected: x10 = 10, x11 = 5, x12 = 15, x13 = 15

# ── Setup base address ──
addi x20, x0, 192

addi x1, x0, 10
addi x2, x0, 5
addi x0, x0, 0
addi x0, x0, 0

# ── Convert and store to memory (NOP-separated) ──
fcvt.s.w f1, x1        # f1 = 10.0
addi x0, x0, 0
addi x0, x0, 0

fcvt.s.w f2, x2        # f2 = 5.0
addi x0, x0, 0
addi x0, x0, 0

fsw f1, 0(x20)          # mem[192] = 10.0
addi x0, x0, 0
addi x0, x0, 0

fsw f2, 4(x20)          # mem[196] = 5.0
addi x0, x0, 0
addi x0, x0, 0

# ── Load-use-store chain (BACK-TO-BACK, no NOPs) ──
flw f10, 0(x20)         # f10 = 10.0 (from memory)
flw f11, 4(x20)         # f11 = 5.0  (from memory)
fadd.s f12, f10, f11    # f12 = 15.0 (load-use hazard on f10 and f11)
fsw f12, 8(x20)         # mem[200] = 15.0 (compute-to-store hazard on f12)
flw f13, 8(x20)         # f13 = 15.0 (load from just-stored location)

# ── Drain and convert ──
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x10, f10      # x10 = 10
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x11, f11      # x11 = 5
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x12, f12      # x12 = 15
addi x0, x0, 0
addi x0, x0, 0

fcvt.w.s x13, f13      # x13 = 15
