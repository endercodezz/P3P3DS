# Parsed by scripts/check_vulkan_stack.py.
include_guard(GLOBAL)

set(REXGLUE_VULKAN_SDK_VERSION "1.4.357.0")

set(REXGLUE_VULKAN_SDK_PINS
    "vulkan-headers=e3b1eec08173d6b825cd3ac88c885a63b621504a"
    "vulkan-loader=5f157b62e333c63260d05d81bf66faa216ab0fb8"
    "moltenvk=db445ff2042d9ce348c439ad8451112f354b8d2a"
    "spirv-headers=29981f65241605e08b0ede4cfeb999fe3b723c6a"
    "spirv-tools=9a49b0883b9b635689a85b5647dbfcb223268151"
)

set(REXGLUE_VULKAN_SDK_EXEMPT
    "glslang"
)
