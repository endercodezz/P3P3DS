
This test (autogen-perftest.c) is generated using ./gen-perf-tests.py
(check out "make tests" for details)

Throughput is calculated by executing an instruction repeatedly and counting
the number of cycles. Note that instructions must be independent to avoid
hazard logic from adding pipeline bubbles.

Latency is caculated by having a dependent instruction immediately after each
instruction. This way we can observe the full pipeline bubble inserted. We
chose vcmp as the blocking instruction since it has a small latency and no
Write-After-Write (WAW) hazards on the control register.

It is important to note that the VFPU pipeline has some WAW hazards: if a short
latency instruction follows a long latency one, this instrucion will not
complete until the previous one has, since the register write order is serial
(to avoid out WAW hazards), even if the output registers do not collide.

