# --- Test 2: EX/MEM Forwarding ---
addi t0, x0, 5     # t0 = 5
add  t1, t0, t0    # t1 = t0 + t0 (t1 = 10)
sub  t2, t1, t0    # t2 = t1 - t0 (t2 = 10 - 5 = 5)

# After running the 3 instructions:
# EXPECT_EQ(cpu.getReg("t0"), 5);
# EXPECT_EQ(cpu.getReg("t1"), 10); // Result of 'add'
# EXPECT_EQ(cpu.getReg("t2"), 5);  // Result of 'sub' (using forwarded t1)