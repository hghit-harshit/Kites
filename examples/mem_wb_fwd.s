# --- Test 3: MEM/WB Forwarding ---
addi t0, x0, 7     # t0 = 7
add  t1, t0, t0    # t1 = t0 + t0 (t1 = 14)
nop                # Stall/bubble
sub  t2, t1, t0    # t2 = t1 - t0 (t2 = 14 - 7 = 7)


# // After running the 4 instructions:
# EXPECT_EQ(cpu.getReg("t0"), 7);
# EXPECT_EQ(cpu.getReg("t1"), 14); // Result of 'add'
# EXPECT_EQ(cpu.getReg("t2"), 7);  // Result of 'sub' (using forwarded t1)