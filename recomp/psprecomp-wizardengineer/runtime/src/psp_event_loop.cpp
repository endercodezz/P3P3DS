#include "psp_event_loop.h"
#include "psp_runtime.h"
#include "psp_render_queue.h"
#include "psp_scheduler.h"
#include "psp_ge_draw.h"

#include <SDL.h>
#include <glad/glad.h>
#include <csignal>
#include <cstdio>
#include <atomic>
#include <chrono>
#include <thread>

// --- Global state definitions (declared extern in psp_runtime.h) ---
std::atomic<bool> g_should_exit{false};
std::atomic<int>  g_alive_threads{0};
std::thread::id   g_main_thread_id;
std::atomic<uint32_t> g_host_buttons{0};

// ---------------------------------------------------------------------------
// Keyboard -> PSP button mapping
//   Arrows -> D-pad | X / Enter -> CROSS | Z -> CIRCLE | A -> SQUARE
//   S -> TRIANGLE   | Q -> LTRIGGER     | W -> RTRIGGER
//   Space -> START  | Tab -> SELECT
// ---------------------------------------------------------------------------
static uint32_t key_to_psp_button(SDL_Keycode key) {
    switch (key) {
        case SDLK_UP:     return 0x0010;  // UP
        case SDLK_RIGHT:  return 0x0020;  // RIGHT
        case SDLK_DOWN:   return 0x0040;  // DOWN
        case SDLK_LEFT:   return 0x0080;  // LEFT
        case SDLK_x:
        case SDLK_RETURN: return 0x4000;  // CROSS
        case SDLK_z:      return 0x2000;  // CIRCLE
        case SDLK_a:      return 0x8000;  // SQUARE
        case SDLK_s:      return 0x1000;  // TRIANGLE
        case SDLK_q:      return 0x0100;  // LTRIGGER
        case SDLK_w:      return 0x0200;  // RTRIGGER
        case SDLK_SPACE:  return 0x0008;  // START
        case SDLK_TAB:    return 0x0001;  // SELECT
        default:          return 0;
    }
}

// Handle one SDL event: window close + keyboard button state.
static void handle_sdl_event(const SDL_Event& ev) {
    if (ev.type == SDL_QUIT) {
        g_should_exit.store(true);
    } else if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
        if (ev.key.repeat) return;  // ignore OS key repeat
        uint32_t bit = key_to_psp_button(ev.key.keysym.sym);
        if (bit == 0) return;
        if (ev.type == SDL_KEYDOWN) {
            g_host_buttons.fetch_or(bit, std::memory_order_relaxed);
        } else {
            g_host_buttons.fetch_and(~bit, std::memory_order_relaxed);
        }
    } else if (ev.type == SDL_WINDOWEVENT &&
               ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
        // KEYUP is not delivered after focus loss; release everything
        // so no button sticks.
        g_host_buttons.store(0, std::memory_order_relaxed);
    }
}

// --- Static state (SDL window + GL context, module-private) ---
static SDL_Window*  g_window     = nullptr;
static SDL_GLContext g_gl_context = nullptr;

// ---------------------------------------------------------------------------
// psp_runtime_init_sdl — create SDL window + GL 3.3 core context + GLAD
// ---------------------------------------------------------------------------
int psp_runtime_init_sdl() {
    // Record main thread id for GL_THREAD_CHECK
    g_main_thread_id = std::this_thread::get_id();

    // 1. Initialize SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr,
            "[RT] SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // 2. Set GL attributes: OpenGL 3.3 core profile, double-buffered
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // 3. Create window at PSP native resolution (480x272)
    g_window = SDL_CreateWindow(
        "PSPrecomp Runtime",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        480, 272,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!g_window) {
        std::fprintf(stderr,
            "[RT] SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // 4. Create GL context (must precede gladLoadGL — Pitfall 6)
    g_gl_context = SDL_GL_CreateContext(g_window);
    if (!g_gl_context) {
        std::fprintf(stderr,
            "[RT] SDL_GL_CreateContext failed: %s\n",
            SDL_GetError());
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return 1;
    }

    // 5. Load GL function pointers via GLAD2 (MUST be after context creation)
    //    GLAD2 requires a loader function; SDL2 provides SDL_GL_GetProcAddress.
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        std::fprintf(stderr, "[RT] gladLoadGL failed\n");
        SDL_GL_DeleteContext(g_gl_context);
        SDL_DestroyWindow(g_window);
        SDL_Quit();
        return 1;
    }

    // 6. Print GL info for diagnostics
    std::fprintf(stderr, "[RT] GL: %s\n",
        glGetString(GL_VERSION));

    // 7. Clear to black and swap — show initial frame
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SwapWindow(g_window);

    std::fprintf(stderr, "[RT] SDL/GL initialized (480x272)\n");
    return 0;
}

// ---------------------------------------------------------------------------
// psp_event_loop — main-thread loop: SDL events + render queue + alive check
// ---------------------------------------------------------------------------
void psp_event_loop(uint8_t* rdram) {
    while (!g_should_exit.load()) {
        // 1. Pump SDL events (window close, keyboard input)
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            handle_sdl_event(ev);
        }

        // 2. Drain render queue — execute pending GL work from game threads
        render_queue_process();

        // 2a. Liveness net (issue #59): present the front buffer if lists
        //     were processed but no real page-flip arrived recently. No-op
        //     when a flip just presented (not dirty / within budget).
        ge_draw_present_if_stale(rdram);

        // 2b. Service any pending debug-socket screenshot request here on
        //     the main (GL) thread — the socket thread never touches GL.
        ge_draw_service_screenshot_request();

        // 3. Check if any game threads are still alive
        if (g_alive_threads.load() == 0) {
            std::fprintf(stderr,
                "[RT] All game threads finished — lingering 3s to flush render\n");
            auto linger_end = std::chrono::steady_clock::now()
                              + std::chrono::seconds(3);
            while (std::chrono::steady_clock::now() < linger_end
                   && !g_should_exit.load()) {
                SDL_Event ev;
                while (SDL_PollEvent(&ev)) {
                    handle_sdl_event(ev);
                }
                render_queue_process();
                ge_draw_present_if_stale(rdram);
                ge_draw_service_screenshot_request();
                SDL_Delay(16);  // ~60fps drain rate
            }
            break;
        }

        // 4. Brief sleep — 1ms balances CPU usage vs responsiveness.
        //    Skip delay when GE lists are pending to drain them faster.
        if (!render_queue_has_pending_ge()) {
            SDL_Delay(1);
        }
    }

    // Final drain to unblock any game thread blocked in render_queue_post
    render_queue_process();
}

// ---------------------------------------------------------------------------
// Signal handling — SIGTERM / SIGINT set g_should_exit
// ---------------------------------------------------------------------------
static void signal_handler(int sig) {
    (void)sig;
    g_should_exit.store(true);
}

void psp_install_signal_handlers() {
    struct sigaction sa{};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);
}

// ---------------------------------------------------------------------------
// psp_runtime_shutdown — clean shutdown: threads, render queue, SDL/GL
// ---------------------------------------------------------------------------
SDL_Window* psp_get_sdl_window() {
    return g_window;
}

void psp_runtime_shutdown() {
    // 1. Ensure exit flag is set (in case called without signal)
    g_should_exit.store(true);

    // 2. Shutdown scheduler — wake all threads, join with timeout
    psp_scheduler_shutdown();

    // 3. Final render queue drain
    render_queue_process();

    // 4. Destroy GL context
    if (g_gl_context) {
        SDL_GL_DeleteContext(g_gl_context);
        g_gl_context = nullptr;
    }

    // 5. Destroy SDL window
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }

    // 6. Quit SDL
    SDL_Quit();

    std::fprintf(stderr, "[RT] Shutdown complete\n");
}
