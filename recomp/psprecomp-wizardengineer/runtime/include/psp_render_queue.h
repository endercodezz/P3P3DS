#pragma once
#include <cstdint>

/// Render request types for the GL-on-main-thread queue.
/// Phase 5 fills in actual GL dispatch for each type.
enum class RenderRequestType : int {
    None = 0,
    DisplayList = 1,   // Phase 5: process single display list
    AllLists = 2,      // Phase 5: process all pending display lists
    FramePresent = 3,  // Phase 5: present framebuffer to window
};

/// Single render request slot (used for FramePresent blocking path).
/// Game thread fills a local RenderRequest, render_queue_post copies it
/// into the global slot and blocks until the main thread completes it.
struct RenderRequest {
    RenderRequestType type = RenderRequestType::None;
    bool pending = false;
    bool done = false;
    uint8_t* rdram = nullptr;
    // Display list fields
    int list_idx = -1;
    uint32_t list_addr = 0;
    uint32_t stall_addr = 0;
    // Frame present fields (Phase 5 extends)
    uint32_t fb_addr = 0;
    uint32_t fb_stride = 0;
    uint32_t fb_format = 0;
};

/// Pending GE display list in the non-blocking FIFO queue.
/// sceGeListEnQueue pushes these; render_queue_process drains them on the
/// main thread where GL calls are safe.
struct GePendingList {
    int uid = 0;
    uint32_t list_addr = 0;
    uint32_t stall_addr = 0;
    uint32_t current_pc = 0;   // Where we left off (for stall-resume)
    uint8_t* rdram = nullptr;
    bool processed = false;
    bool stalled = false;      // True if hit stall, waiting for update
    bool begun = false;        // True if ge_draw_begin_list was called
};

/// Game thread side: post FramePresent request and block until main thread
/// completes it. Uses unbounded condvar wait intentionally -- the game
/// thread MUST block until GL work completes. The safety valve is
/// g_should_exit (checked in the wait predicate), not a timeout.
void render_queue_post(RenderRequest& req);

/// Non-blocking GE list enqueue. Returns a list UID immediately.
/// The game thread can continue CPU work after this call.
/// The main thread will process the list via render_queue_process().
int render_queue_enqueue_ge_list(
    uint8_t* rdram, uint32_t list_addr, uint32_t stall_addr);

/// Synchronize with GE list processing.
/// mode=0: block until ALL pending GE lists are processed (returns 0).
/// mode=1: poll -- returns 0 if all done, 1 if still processing.
int render_queue_draw_sync(int mode);

/// Update the stall address of an already-enqueued list.
/// Called from sceGeListUpdateStallAddr to advance the stall.
int render_queue_update_stall(int uid, uint32_t new_stall);

/// Returns true if no pending GE lists in the queue.
bool render_queue_all_ge_done();

/// Returns true if there are pending GE lists waiting to be processed.
bool render_queue_has_pending_ge();

/// Main thread side: process pending FramePresent request (blocking path)
/// AND drain ALL pending GE lists from the FIFO (non-blocking path).
/// Returns immediately if nothing is pending.
void render_queue_process();
