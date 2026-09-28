#include "psp_memory.h"
#include <sys/mman.h>
#include <cstdio>

uint8_t* psp_memory_init() {
    // macOS uses MAP_ANON (not MAP_ANONYMOUS).
    // MAP_PRIVATE | MAP_ANON creates a sparse allocation — only touched
    // pages consume physical memory. This avoids committing the full 128MB.
    void* mem = mmap(
        nullptr,
        PSP_MEM_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANON,
        -1,
        0
    );

    if (mem == MAP_FAILED) {
        std::fprintf(stderr, "psp_memory_init: mmap(%zu bytes) failed\n",
                     PSP_MEM_SIZE);
        return nullptr;
    }

    return static_cast<uint8_t*>(mem);
}

void psp_memory_cleanup(uint8_t* rdram) {
    if (rdram) {
        munmap(rdram, PSP_MEM_SIZE);
    }
}
