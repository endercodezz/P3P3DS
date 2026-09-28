#pragma once
#include <cstdint>

/// SDL2+GL initialization: creates window, GL context, loads GLAD.
/// Must be called from main thread before any GL calls.
/// Returns 0 on success, non-zero on failure.
int psp_runtime_init_sdl();

/// Main event loop: processes SDL events, drains render queue, checks alive
/// threads. Blocks until g_should_exit is true or all game threads exit.
void psp_event_loop(uint8_t* rdram);

/// Install SIGTERM/SIGINT signal handlers that set g_should_exit.
void psp_install_signal_handlers();

/// Full shutdown: drain render queue, join threads, destroy SDL window/GL
/// context.
void psp_runtime_shutdown();

// Forward declare SDL_Window to avoid SDL.h in header
struct SDL_Window;

/// Get the SDL window pointer (for FBO blit and swap).
SDL_Window* psp_get_sdl_window();
