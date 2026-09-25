
This is a collection of tests that validate some corner cases, bugs and other
assertions about the VFPU behaviour present in the documentation.

 - ieee754-fun.c: Checks some IEEE754 corner cases. Performs some validation
   of the NaN/Inf encodings as well as verifies that their are produced when
   the standard indicates so. Validates that VFPU always rounds to nearest,
   that denormals are handled as zero and some interesting bugs related to
   Nan <-> Inf mappings.

 - test-conditional.c: Tests the behavior of the VFPU_CC register. Proves
   that mfvc has a non-interlocked hazard, but that branch instructions have
   not. Checks some basic vcmp behaviour. TODO: check nan/inf/-0 comparisons.

 - vfpu-bugs.c: Validates the behaviour of certain instructions known to be
   buggy.

 - vfpu-cc.c: Tests the behaviour of the VFPU CC registers. Validates
   the behaviour of the prefix instructions and the prefix stack regsisters.
   Checks some basic behaviour of the random registers. VFPU_CC is excluded.

 - memops.c and memops-helper.S: Tests the behaviour of lvr.q/lvl.q
   instructions and their silicon bugs. The tests have different expectations
   depending on the PSP model where this is executed. Also test some alignment
   requirements for svl/svr/lvl/lvr instructions.

