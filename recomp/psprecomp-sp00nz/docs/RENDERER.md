# Rendering

How the GE gets to a window, and why the backend is an interface rather than a
Vulkan file.

## The shape of the problem

The GE is not a function you call. User code builds a **display list** — 32-bit
words, each an 8-bit command and 24 bits of argument — and hands the GE a pointer
plus a stall address. The GE consumes commands up to the stall; the CPU moves
the stall forward as it writes more.

So `sceGu*`, the list-building library, is ordinary user code and gets
recompiled like anything else. Only list *execution* is ours. That execution has
two halves, and they are worth keeping apart:

| Half | What it does | Where it lives |
|---|---|---|
| **Interpretation** | Walk the list, follow JUMP/CALL/RET/END/FINISH, track state, assemble primitives | `src/hle/ge.c` — one implementation, always |
| **Presentation** | Turn assembled primitives into pixels | a *backend*, chosen at run time |

Everything upstream of a triangle is the same regardless of how the triangle is
drawn. Mixing the two produces a renderer that cannot be tested without a GPU
and cannot be diffed against a reference — which is the thing that matters most
during bring-up.

## Why a backend interface

Three reasons, in order of how much they cost to get wrong:

1. **The software path must survive.** It is the oracle. A GPU backend that
   disagrees with it is wrong, and you cannot establish that if the GPU backend
   is the only one. `tests/test_raster.c` asserts *where* pixels land using
   nothing but the software path, and that must keep working on a machine with
   no GPU at all — CI included.

2. **A wrong pixel is harder to debug than a missing one.** During bring-up the
   question is "did the game ask to draw this", not "does it look right". A
   backend boundary lets the answer be recorded (command counts, primitive
   counts, vertex counts) independently of whether anything was presented.

3. **Portability is the point of the project.** Vulkan is one answer. It should
   not be the only place the GE's semantics are written down.

## The interface

```c
typedef struct {
    const char *name;

    int  (*init)(int width, int height);
    void (*shutdown)(void);

    /* Framebuffer target changed (GE_FBP/GE_FBW). */
    void (*set_target)(uint32_t addr, uint32_t stride, int fmt);

    /* One assembled primitive, in screen space, already clipped to the
     * viewport by the interpreter. Vertices carry position and colour;
     * texture support extends this struct rather than replacing it. */
    void (*draw)(int prim, const psp_vertex *v, int count);

    /* End of a display list; a good point to flush batched work. */
    void (*finish)(void);

    /* Present whatever has accumulated (sceDisplaySetFrameBuf). */
    void (*present)(void);
} psp_render_backend;
```

`draw` takes *assembled* primitives rather than raw display-list words on
purpose. Vertex format decoding — the stride arithmetic, the component
alignment, through-mode vs transformed — is fiddly and is exactly the part that
must not be duplicated per backend. Get it wrong once, centrally, and every
backend is wrong the same way, which is at least diagnosable.

## Backends

| Backend | State | Notes |
|---|---|---|
| **software** | Working | The reference. Triangles, strips, sprites in through-mode; six tests assert pixel positions. No GPU, no dependencies, runs in CI. |
| **null** | Working | Counts primitives, draws nothing. What the bring-up host uses when the question is "did it ask to draw". |
| **sdl3-vulkan** | Not started | The intended presentation path. See below. |

## The SDL3 + Vulkan backend, when it happens

Scope it deliberately, because the GE has a large state space and most of it
does not matter until a game is already drawing:

**First increment** — enough to see the software path's output in a window:
- SDL3 window + swapchain
- Upload the emulated framebuffer as a texture, blit, present

That is not really a GPU backend — it is a *presenter* for the software
rasterizer. It is worth doing first anyway, because it makes everything after it
visible, and because "the software renderer's output, on screen" is a milestone
with no ambiguity about whether it worked.

**Second increment** — geometry on the GPU:
- Vertex buffer per display list, one draw per primitive batch
- Through-mode only, matching what the software path already does
- Diff against the software path on the same list: same pixels, or the backend
  is wrong

**Third increment** — the parts that need real work:
- Texture cache keyed on the GE's texture state (address, format, size, CLUT)
- The transform pipeline, which needs the VFPU matrices to be correct first
- Blending, depth, scissor — each is a small addition once the above holds

## Validation

The software backend is checked against hand-built display lists in
`tests/test_raster.c` — synthetic, no game data, asserting pixel positions rather
than pixel counts. A backend that fills the whole screen and one that fills the
right rectangle both report a nonzero count; only one is correct.

Any GPU backend is checked against the software backend on the same list. That
is the same oracle arrangement the rest of the project uses (see
[ORACLE.md](ORACLE.md)): a reference you diff against, not a dependency you
link.

## Prior art

[sal063/PSP-recompilation-project](https://github.com/sal063/PSP-recompilation-project)
independently built an SDL3 + Vulkan GE backend, validated against PPSSPP's
software renderer. It demonstrates the approach is sound.

This design was written from the GE's documented behaviour and the shape of our
existing interpreter, not from reading theirs — deliberately, because that
project incorporates GPL code from PPSSPP in parts of its HLE and psprecomp
keeps a hard MIT boundary. Their *result* is evidence the problem is tractable;
their *implementation* is not something this project can borrow from.
