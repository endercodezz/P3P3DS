
.text
.set noreorder

.global do_test_lvl
.global do_test_lvr
.global do_test_ulv

# Some read only data for testing (cache line aligned)
.align 6
testdataptr:
.word 0x3ff0030f, 0x3ff1031f, 0x3ff2032f, 0x3ff3033f
.word 0x3ff4034f, 0x3ff5035f, 0x3ff6036f, 0x3ff7037f
.word 0x3ff8038f, 0x3ff9039f, 0x3ffa03af, 0x3ffb03bf
.word 0x3ffc03cf, 0x3ffd03df, 0x3ffe03ef, 0x3fff03ff
.word 0x3ff0f30f, 0x3ff1f31f, 0x3ff2f32f, 0x3ff3f33f
.word 0x3ff4f34f, 0x3ff5f35f, 0x3ff6f36f, 0x3ff7f37f

#define fillreg(nreg)                  \
  li $v0, (0x449a0000 | (nreg << 8));  \
  mtc1 $v0, $f##nreg;

# Fills all regs with some well-known FP32 value
reset_fpu_regs:
  fillreg( 0); fillreg( 1); fillreg( 2); fillreg( 3);
  fillreg( 4); fillreg( 5); fillreg( 6); fillreg( 7);
  fillreg( 8); fillreg( 9); fillreg(10); fillreg(11);
  fillreg(12); fillreg(13); fillreg(14); fillreg(15);
  fillreg(16); fillreg(17); fillreg(18); fillreg(19);
  fillreg(20); fillreg(21); fillreg(22); fillreg(23);
  fillreg(24); fillreg(25); fillreg(26); fillreg(27);
  fillreg(28); fillreg(29); fillreg(30); fillreg(31);

  jr $ra
  nop


# Just saves the regs into an array for later inspection
# Writes 128 bytes

#define savereg(nreg)               \
  mfc1 $v0, $f##nreg;               \
  sw $v0, (nreg)*4 ($s0);

# Fills all regs with some well-known FP32 value
save_fpu_regs:
  savereg( 0); savereg( 1); savereg( 2); savereg( 3);
  savereg( 4); savereg( 5); savereg( 6); savereg( 7);
  savereg( 8); savereg( 9); savereg(10); savereg(11);
  savereg(12); savereg(13); savereg(14); savereg(15);
  savereg(16); savereg(17); savereg(18); savereg(19);
  savereg(20); savereg(21); savereg(22); savereg(23);
  savereg(24); savereg(25); savereg(26); savereg(27);
  savereg(28); savereg(29); savereg(30); savereg(31);

  jr $ra
  nop


# Check which registers where modified from their original value
# Returns a bit mask with the non-matching registers (0 means OK)
check_fpu_regs:
  move $v0, $0

#define checkreg(nreg)                  \
  li $a0, (0x449a0000 | (nreg << 8));   \
  mfc1 $v1, $f##nreg;                   \
  beq $a0, $v1, 1f;                     \
  sll $v0, $v0, 1;                      \
  ori $v0, 1;                           \
 1:

  checkreg(31); checkreg(30); checkreg(29); checkreg(28)
  checkreg(27); checkreg(26); checkreg(25); checkreg(24)
  checkreg(23); checkreg(22); checkreg(21); checkreg(20)
  checkreg(19); checkreg(18); checkreg(17); checkreg(16)
  checkreg(15); checkreg(14); checkreg(13); checkreg(12)
  checkreg(11); checkreg(10); checkreg( 9); checkreg( 8)
  checkreg( 7); checkreg( 6); checkreg( 5); checkreg( 4)
  checkreg( 3); checkreg( 2); checkreg( 1); checkreg( 0)

  jr $ra
  nop

#define ex_lvlq(reg)   lvl.q reg, 0($a1)
#define ex_lvrq(reg)   lvr.q reg, 0($a1)
#define ex_ulvq(reg)   ulv.q reg, 0($a1)

# We load a FPU register to sort of "force" register corruption
# If we load the $f31 reg the last one, we never "see" f31 corruption
# since it gets corrupted with the same value (so no corruption!)

#define dotest(inst, reg, rndreg) \
  jal reset_fpu_regs;             \
  nop;                            \
  fillreg(rndreg);                \
  ex_##inst(reg);                 \
  jal save_fpu_regs;              \
  nop;                            \
  addiu $s0, $s0, 4*32;

#define test_body(inst)    \
  dotest(inst, C000,  1)   \
  dotest(inst, C010,  2)   \
  dotest(inst, C020,  3)   \
  dotest(inst, C030,  4)   \
  dotest(inst, C100,  5)   \
  dotest(inst, C110,  6)   \
  dotest(inst, C120,  7)   \
  dotest(inst, C130,  8)   \
  dotest(inst, C200,  9)   \
  dotest(inst, C210, 10)   \
  dotest(inst, C220, 11)   \
  dotest(inst, C230, 12)   \
  dotest(inst, C300, 13)   \
  dotest(inst, C310, 14)   \
  dotest(inst, C320, 15)   \
  dotest(inst, C330, 16)   \
  dotest(inst, C400, 17)   \
  dotest(inst, C410, 18)   \
  dotest(inst, C420, 19)   \
  dotest(inst, C430, 20)   \
  dotest(inst, C500, 21)   \
  dotest(inst, C510, 22)   \
  dotest(inst, C520, 23)   \
  dotest(inst, C530, 24)   \
  dotest(inst, C600, 25)   \
  dotest(inst, C610, 26)   \
  dotest(inst, C620, 27)   \
  dotest(inst, C630, 28)   \
  dotest(inst, C700, 29)   \
  dotest(inst, C710, 30)   \
  dotest(inst, C720, 31)   \
  dotest(inst, C730,  0)   \
  dotest(inst, R000,  1)   \
  dotest(inst, R001,  2)   \
  dotest(inst, R002,  3)   \
  dotest(inst, R003,  4)   \
  dotest(inst, R100,  5)   \
  dotest(inst, R101,  6)   \
  dotest(inst, R102,  7)   \
  dotest(inst, R103,  8)   \
  dotest(inst, R200,  9)   \
  dotest(inst, R201, 10)   \
  dotest(inst, R202, 11)   \
  dotest(inst, R203, 12)   \
  dotest(inst, R300, 13)   \
  dotest(inst, R301, 14)   \
  dotest(inst, R302, 15)   \
  dotest(inst, R303, 16)   \
  dotest(inst, R400, 17)   \
  dotest(inst, R401, 18)   \
  dotest(inst, R402, 19)   \
  dotest(inst, R403, 20)   \
  dotest(inst, R500, 21)   \
  dotest(inst, R501, 22)   \
  dotest(inst, R502, 23)   \
  dotest(inst, R503, 24)   \
  dotest(inst, R600, 25)   \
  dotest(inst, R601, 26)   \
  dotest(inst, R602, 27)   \
  dotest(inst, R603, 28)   \
  dotest(inst, R700, 29)   \
  dotest(inst, R701, 30)   \
  dotest(inst, R702, 31)   \
  dotest(inst, R703,  0)


# Tests lvl instruction and its FPU reg destruction potential
# arg0: register corruption array (32x32b)
do_test_lvl:
  addiu $sp, $sp, -16
  sw $ra, 0($sp)
  sw $s0, 4($sp)
  move $s0, $a0           # Save res pointer

  # Use an aligned valid address
  la $a2, testdataptr
  addu $a1, $a1, $a2

  test_body(lvlq)

  lw $ra, 0($sp)
  lw $s0, 4($sp)
  jr $ra
  addiu $sp, $sp, 16


# Tests lvr instruction and its FPU reg destruction potential
# arg0: register corruption array (32x32b)
do_test_lvr:
  addiu $sp, $sp, -16
  sw $ra, 0($sp)
  sw $s0, 4($sp)
  move $s0, $a0           # Save res pointer

  # Use an aligned valid address
  la $a2, testdataptr
  addu $a1, $a1, $a2

  test_body(lvrq)

  lw $ra, 0($sp)
  lw $s0, 4($sp)
  jr $ra
  addiu $sp, $sp, 16


# Tests lvr+lvl (ulv) instruction and its FPU reg destruction potential
# arg0: register corruption array (32x32b)
do_test_ulv:
  addiu $sp, $sp, -16
  sw $ra, 0($sp)
  sw $s0, 4($sp)
  move $s0, $a0           # Save res pointer

  # Use an aligned valid address
  la $a2, testdataptr
  addu $a1, $a1, $a2

  test_body(ulvq)

  lw $ra, 0($sp)
  lw $s0, 4($sp)
  jr $ra
  addiu $sp, $sp, 16


