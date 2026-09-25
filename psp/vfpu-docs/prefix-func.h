
// Implementation/Emulation of prefix operand mangling
// Described in a way that potentially resembles the hardware.
// Likely there's a 4:1 mux to choose operand (swizzle) followed by
// a gate to clear the MSB (for abs() operation).
// In parallel these three selection bits are used to select a constant
// and finally these two partial results are selected by a 2:1 mux.
// Final operation is a simple xor of the MSB to negate operand.

// Evidence of this is the variety of patterns that can be obtained:
//  -|x| being possible means abs() goes before negation
//  -/+ constant means that constants go before negation
// absolute bit being reused for constant offset means that constant
// and absolute logic likely work in parallel (same for swizzle).

// This is nice and useful, since things like -|x| can be calculated,
// as well as doubling the constant pool size (positive and negative).

// Some instructions are (ab)using the prefix logic to perform operations:
//
// - vscl: Seems to be a vmul + swizzle on rt input.
// - vabs: Probably hardcodes abs/neg bits (allows swizzle)
// - vneg: Like vabs but a different subset of bits
// - vn{sin,rcp,...} Likely abusing the neg bit.
//    (since sin(-x) = -sin(x), vrexp2(x) = vexp2(-x), etc.
//     likely the reason why no vncos, vrlog2, ...)
//    
// Thus this operations have some limited prefixes available.

static const uint32_t prefix_cnt[8] = {
	0x00000000,    // 0.0f
	0x3f800000,    // 1.0f
	0x40000000,    // 2.0f
	0x3f000000,    // 0.5f
	0x40400000,    // 3.0f
	0x3eaaaaab,    // 1/3
	0x3e800000,    // 0.25f
	0x3e2aaaab,    // 1/6
};

static inline void pfx_src(uint32_t *output, uint32_t *input, uint32_t pfx, unsigned lanen) {
	for (unsigned i = 0; i < lanen; i++) {
		// Bits 15..12 specify constant (1) or swizzle+abs (0)
		unsigned cntop = ((pfx >> 12) >> i) & 1;

		// Uses the 8 LSB in groups of 2 bits to indicate swizzle index
		unsigned swsel = (pfx >> (i << 1)) & 3;
		// Bits 11..8 specify abs operation
		unsigned absel = ((pfx >> 8) >> i) & 1;

		uint32_t value;

		if (cntop) {
			// It (ab)uses the swizzle and abs bits to pick the constant
			unsigned ctsel = swsel | (absel << 2);
			// Calculate constant based on ctsel
			value = prefix_cnt[ctsel];
		} else {
			// Use swizzle bits to select input
			uint32_t swz = input[swsel];
			// Perform abs() if specified (clears sign bit)
			value = absel ? (swz & 0x7FFFFFFF) : swz;
		}

		// Bits 19..16 specifiy operand negation
		unsigned negat = ((pfx >> 16) >> i) & 1;

		// Negate if required
		output[i] = negat ? (value ^ 0x80000000) : value;
	}
}

// Destination prefix consists in 3 bits per lane.
// One bit determines whether the register will be written at all (mask).
// The other two are used for saturation operations: one bit enables saturation
// while the other chooses the range (0:1 vs -1:1).
// Some combinations are legal but do not make sense:
//  - mask + saturation: don't care about the result since it's not written
//  - saturation selection bit + no-saturation
//
// This suggests that saturation is performed before the masking (likely masking
// just disables register updates) and that the range is selected by one bit while
// the other selects whether to apply the operation or not.

static inline void pfx_dst(uint32_t *output, uint32_t *input, uint32_t pfx, unsigned lanen) {
	for (unsigned i = 0; i < lanen; i++) {
		// Uses the 8 LSB in groups of 2 bits (1 bit for sat, 1 for minsat)
		unsigned dosat = (pfx >> (i * 2)) & 1;
		unsigned satsl = (pfx >> (i * 2 + 1)) & 1;
		// Bits 11..8 specify masking operation (1 means do not write)
		unsigned domask = ((pfx >> 8) >> i) & 1;

		if (!domask) {
			if (dosat) {
				union {
					float f32;
					uint32_t u32;
				} v = {.u32 = input[i]};
				const float maxv = 1.0f;
				const float minv = satsl ? -1.0f : 0.0f;
				if (v.f32 > maxv)
					v.f32 = maxv;
				if (v.f32 <= minv)   // Ensure we map -0 to 0
					v.f32 = minv;
				output[i] = v.u32;
			} else {
				output[i] = input[i];
			}
		}
	}
}


