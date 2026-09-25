
This test (autogen-regtests.c) is generated using ./gen-regtests.py
(check out "make tests" for details)

Tests all instructions with all possible register permutations (except for
input overlap, since it is tricky and is not useful for this test). The test
compares the results with a previous reference test that uses some
non-overlapping registers. If the results differ, the error is classified
between two possible explanations:

 - The input-output overlap can be causing it
 - There's some CPU bug

The tests show no bugs except for vhtfm2 and vhtfm3 instructions.

The overlap is further classified in overlap types. Some instructions allow
certain amount of overlap, whereas some others not at all.

