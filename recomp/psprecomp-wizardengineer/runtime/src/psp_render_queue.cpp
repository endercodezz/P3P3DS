#include "psp_render_queue.h"
#include "psp_runtime.h"
#include "psp_ge.h"
#include "psp_ge_draw.h"
#include "psp_scheduler.h"
#include "hle/psp_hle.h"
#include "recomp.h"

#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstdio>
#include <chrono>

// ---------------------------------------------------------------------------
// Shared mutex and condvar (used by both blocking single-slot and GE FIFO)
// ---------------------------------------------------------------------------
static std::mutex g_render_mutex;
static std::condition_variable g_render_cv;

// ---------------------------------------------------------------------------
// Single-slot blocking path (FramePresent)
// ---------------------------------------------------------------------------
static RenderRequest g_render_req;

// ---------------------------------------------------------------------------
// Multi-slot non-blocking GE list FIFO
// ---------------------------------------------------------------------------
static std::vector<GePendingList> g_ge_queue;
static std::condition_variable g_ge_done_cv;
// Start at 1: PPSSPP returns 1-based GE list IDs; game passes the
// returned uid back to UpdateStallAddr so uid=0 causes a mismatch.
static int g_ge_uid_counter = 1;

// ---------------------------------------------------------------------------
// render_queue_post -- blocking single-slot path for FramePresent
// ---------------------------------------------------------------------------
void render_queue_post(RenderRequest& req) {
    std::unique_lock<std::mutex> lock(g_render_mutex);

    g_render_req.type = req.type;
    g_render_req.rdram = req.rdram;
    g_render_req.list_idx = req.list_idx;
    g_render_req.list_addr = req.list_addr;
    g_render_req.stall_addr = req.stall_addr;
    g_render_req.fb_addr = req.fb_addr;
    g_render_req.fb_stride = req.fb_stride;
    g_render_req.fb_format = req.fb_format;
    g_render_req.pending = true;
    g_render_req.done = false;

    g_render_cv.notify_one();

    g_render_cv.wait(lock, [] {
        return g_render_req.done || g_should_exit.load();
    });
}

// ---------------------------------------------------------------------------
// render_queue_enqueue_ge_list -- non-blocking GE list submission
// ---------------------------------------------------------------------------
int render_queue_enqueue_ge_list(
    uint8_t* rdram, uint32_t list_addr, uint32_t stall_addr
) {
    std::unique_lock<std::mutex> lock(g_render_mutex);

    int uid = g_ge_uid_counter++;

    GePendingList entry;
    entry.uid = uid;
    entry.list_addr = list_addr;
    entry.stall_addr = stall_addr;
    entry.current_pc = list_addr;
    entry.rdram = rdram;
    entry.processed = false;
    entry.stalled = false;
    entry.begun = false;

    g_ge_queue.push_back(entry);

    g_render_cv.notify_one();

    return uid;
}

// ---------------------------------------------------------------------------
// render_queue_update_stall -- advance stall address for a pending list
// ---------------------------------------------------------------------------
int render_queue_update_stall(int uid, uint32_t new_stall) {
    std::unique_lock<std::mutex> lock(g_render_mutex);

    for (auto& entry : g_ge_queue) {
        if (entry.uid == uid) {
            entry.stall_addr = new_stall;
            entry.stalled = false;  // Unstall so it gets re-processed
            g_render_cv.notify_one();
            return 0;
        }
    }

    // UID not found (already completed or invalid) -- log to surface mismatches
    static int miss_count = 0;
    if (miss_count < 10) {
        std::fprintf(stderr,
            "[GE] UpdateStallAddr: uid=%d not found in queue "
            "(queue size=%zu)\n",
            uid, g_ge_queue.size());
        miss_count++;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// render_queue_draw_sync -- synchronize with GE processing
// ---------------------------------------------------------------------------
int render_queue_draw_sync(int mode) {
    // Yield first (no mutex held) so the render thread can process lists
    // before this thread blocks waiting for them to complete.
    sched_yield_point();

    std::unique_lock<std::mutex> lock(g_render_mutex);

    if (mode == 0) {
        // Blocking: wait until all GE lists are drained (500ms safety valve).
        // Timeout prevents infinite hang when a list is stalled with no
        // further UpdateStallAddr call (e.g. uid mismatch or lost list).
        bool drained = g_ge_done_cv.wait_for(lock,
            std::chrono::milliseconds(500),
            [] { return g_ge_queue.empty() || g_should_exit.load(); });
        if (!drained) {
            std::fprintf(stderr,
                "[GE] DrawSync timeout: %zu lists still pending\n",
                g_ge_queue.size());
        }
        return 0;
    }

    // mode == 1: poll -- return 0 if done, 1 if still processing
    return g_ge_queue.empty() ? 0 : 1;
}

// ---------------------------------------------------------------------------
// render_queue_all_ge_done / render_queue_has_pending_ge
// ---------------------------------------------------------------------------
bool render_queue_all_ge_done() {
    std::unique_lock<std::mutex> lock(g_render_mutex);
    return g_ge_queue.empty();
}

bool render_queue_has_pending_ge() {
    std::unique_lock<std::mutex> lock(g_render_mutex);
    return !g_ge_queue.empty();
}

// ---------------------------------------------------------------------------
// render_queue_process -- main thread: drain both paths
// ---------------------------------------------------------------------------
void render_queue_process() {
    std::unique_lock<std::mutex> lock(g_render_mutex);

    // --- Path 1: Single-slot blocking request (FramePresent) ---
    if (g_render_req.pending) {
        if (g_should_exit.load()) {
            g_render_req.pending = false;
            g_render_req.done = true;
            g_render_cv.notify_one();
        } else {
            switch (g_render_req.type) {
            case RenderRequestType::FramePresent:
                ge_present_frame(
                    g_render_req.rdram,
                    g_render_req.fb_addr,
                    g_render_req.fb_stride,
                    g_render_req.fb_format);
                break;
            case RenderRequestType::DisplayList:
            case RenderRequestType::AllLists:
                ge_process_display_list(
                    g_render_req.rdram,
                    g_render_req.list_addr,
                    g_render_req.stall_addr);
                break;
            case RenderRequestType::None:
            default:
                break;
            }

            g_render_req.pending = false;
            g_render_req.done = true;
            g_render_cv.notify_one();
        }
    }

    // --- Path 2: Non-blocking GE list FIFO ---
    if (!g_ge_queue.empty() && !g_should_exit.load()) {
        // Process lists in-place. Completed lists get removed;
        // stalled lists stay in the queue for later resume.
        // Release lock during GL calls to avoid blocking game threads.
        // Make a copy of entries to process so we can unlock.
        std::vector<GePendingList> to_process;
        std::vector<int> completed_indices;

        for (size_t i = 0; i < g_ge_queue.size(); i++) {
            auto& entry = g_ge_queue[i];
            if (!entry.stalled) {
                to_process.push_back(entry);
            }
        }

        lock.unlock();

        for (auto& entry : to_process) {
            GeListResult result = ge_process_display_list(
                entry.rdram,
                entry.current_pc,
                entry.stall_addr);

            entry.current_pc = result.stopped_pc;
            if (result.completed) {
                entry.processed = true;
            } else {
                entry.stalled = true;
            }
        }

        lock.lock();

        // Update queue entries with results from processing
        for (auto& processed : to_process) {
            for (auto& queued : g_ge_queue) {
                if (queued.uid == processed.uid) {
                    queued.current_pc = processed.current_pc;
                    queued.processed = processed.processed;
                    queued.stalled = processed.stalled;
                    queued.begun = true;
                    break;
                }
            }
        }

        // Remove completed lists
        g_ge_queue.erase(
            std::remove_if(g_ge_queue.begin(), g_ge_queue.end(),
                [](const GePendingList& e) { return e.processed; }),
            g_ge_queue.end());

        // Notify any thread blocked in render_queue_draw_sync.
        // NOTE: the GE finish callback is NOT fired here. On real PSP the
        // finish callback fires per-list at the FINISH command, which is
        // exactly what psp_ge.cpp's GE_CMD_FINISH path does. Firing it
        // again at queue-drain time was a mistimed double-fire (issue #10).
        if (g_ge_queue.empty()) {
            g_ge_done_cv.notify_all();
        }
    } else if (g_should_exit.load() && !g_ge_queue.empty()) {
        g_ge_queue.clear();
        g_ge_done_cv.notify_all();
    }
}
