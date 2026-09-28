#pragma once
#include <cstdint>

/// Run the GE solid-fill self-test. Writes a hand-crafted display
/// list into rdram, processes it through the full GE pipeline, and
/// verifies the rendered output matches the expected solid color.
/// Returns true if center pixel matches expected color.
bool ge_run_self_test(uint8_t* rdram);
