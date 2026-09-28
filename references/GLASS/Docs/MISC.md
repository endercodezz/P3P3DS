# Docs

## Differences from the standard

- Fragment pipeline is not programmable.
- No default objects are provided (framebuffer, textures, shader program).
- `glBind*` functions will not create buffers when passing arbitrary buffer values, instead `glGen*` must be used for buffer creation.
- Unlike the OpenGL specs which limit each shader type to 1 entry, `glShaderBinary` supports as much shader entries as contained in the shader binary, and are loaded in the order they've been linked.
- `glPolygonOffset`: the `factor` argument has no effect.
- `glDrawArrays`: the `mode` argument must be one of `GL_TRIANGLES`, `GL_TRIANGLE_STRIP`, `GL_TRIANGLE_FAN`, `GL_GEOMETRY_PRIMITIVE_PICA`.
- If the number of components of the uniform variable as defined in the shader does not match the size specified in the name of the command used to load its value, and the uniform variable is not of type `bool`, no error will be generated; the other components of the specified uniform variable will remain unchanged.
- `glClear`: this command doesn't respect the draw order; additionally, scissor test and buffer writemasks are ignored.
- `glTexParameter`: `GL_TEXTURE_MIN_LOD`, `GL_TEXTURE_MAX_LOD`, `GL_TEXTURE_LOD_BIAS` are all valid parameters.
- `glTexParameter`: `GL_CLAMP_TO_BORDER` is a valid wrapping value.
- `glBindAttribLocation`: always returns `GL_INVALID_OPERATION`.
- `glEnable`, `glDisable`, `glIsEnabled` accept additional parameter names: `GL_SCISSOR_TEST_INVERTED_PICA`.
- `glGet*` accepts additional parameter names: `GL_FRAMEBUFFER_BINDING_PICA`, `GL_SCISSOR_TEST_INVERTED_PICA`.
- `glPixelStorei` doesn't support alignment by 8, and all other alignment values are effectively the same (width being >= 8 and po2 guarantees the alignment).

## Attributes

The GPU has 16 input registers, but at most 12 can be used at the same time: `glEnableVertexAttribArray` will fail with `GL_INVALID_OPERATION` if all 12 are used. To use a different register first call `glDisableVertexAttribArray` to disable one of the used registers.

When calling `glVertexAttribPointer`, the `normalized` argument must be set to `GL_FALSE`, otherwise the function fails with `GL_INVALID_OPERATION`; the same error is triggered if a raw buffer is passed as the `pointer` argument, and said buffer is not allocated on linear heap.

In a vertex buffer, each component value must be aligned to its type size; `glVertexAttribPointer` fails with `GL_INVALID_OPERATION` if this is not the case.

## Textures

3 texture units are available. Only `GL_TEXTURE0` can load cube maps, and only one target at time can be used.

A texture width and height must be at least 8, and must not be more than 1024. This implies that there are at most 8 mipmap levels, with the highest possible one being 8x8 (and not 1x1 as described by the standard).

When specifying LOD values, they will be clamped in the range `[0, 15]`. When specifying a LOD bias, it will be clamped in the range `[-15.255, 15.255]`.

`GL_NEAREST` and `GL_NEAREST_MIPMAP_NEAREST` are the same, the GPU decides to use one or the other based on the value of `GL_TEXTURE_MIN_LOD` (ie. whether the filter is active). Same goes for `GL_LINEAR` and `GL_LINEAR_MIPMAP_NEAREST`.

## Combiners

Fragment pipeline can be controlled through combiners. There are 6 combiner stages: each one of them has 2 sources, for both color and alpha. Each of the two has 3 inputs, and an operation can be applied on them. Finally, the outputs are combined, and the result can be used in next stages.

It is possible to control the buffer associated with combiners by setting an initial color through `glCombinerBufferColorPICA`, and by keeping it for all stages or optionally replacing it through `glCombinerBufferInputPICA`.

By default all stages are set as passthrough, all sources are the vertex color, and the combiner buffer color is white.

## Memory allocation

Internally, GLASS has to allocate various kinds of memory buffers to work. It's possible to customize allocation by redefining any of the following functions:

```c
void* glassVirtualAlloc(size_t size);
void glassVirtualFree(void* p);
size_t glassVirtualSize(void* p);
bool glassIsVirtual(void* p);

void* glassLinearAlloc(size_t size);
void glassLinearFree(void* p);
size_t glassLinearSize(void* p);
bool glassIsLinear(void* p);

void* glassVRAMAlloc(size_t size, vramAllocPos pos);
void glassVRAMFree(void* p);
size_t glassVRAMSize(void* p);
bool glassIsVRAM(void* p);
```

- `*Alloc` allocates `size` bytes.
- `*Free` frees the memory pointed by `p` (null pointers are ignored).
- `*Size` returns the usuable size for the memory pointed by `p`.
- `*Is*` returns whether the address is of the specified kind.

By redefining any of the functions above, you will override default memory management routines. This is useful for tracking, controlling and customizing allocations.

If you need to rely on the default behaviour, you can declare and call any of the following functions:

```c
void* GLASS_virtualAllocDefault(size_t size);
void GLASS_virtualFreeDefault(void* p);
size_t GLASS_virtualSizeDefault(void* p);
bool GLASS_isVirtualDefault(void* p);

void* GLASS_linearAllocDefault(size_t size);
void GLASS_linearFreeDefault(void* p);
size_t GLASS_linearSizeDefault(void* p);
bool GLASS_isLinearDefault(void* p);

void* GLASS_vramAllocDefault(size_t size, vramAllocPos pos);
void GLASS_vramFreeDefault(void* p);
size_t GLASS_vramSizeDefault(void* p);
bool GLASS_isVramDefault(void* p);
```

## Debugging

If GLASS doesn't work as intended, or is responsible for crashing applications, you can compile it in debug mode. Assertions will be enabled, and informations will be logged under `sdmc:/GLASS.log`.

By default, GLASS in debug mode will abort whenever an OpenGL error is raised. This behaviour can be controlled with the `GLASS_NO_MERCY` macro.

Note that GLASS only does error checking to a bare minimum (eg. anything specified by the OpenGL ES standard), for space and computation efficiency. It will not check for buffers to be valid, nor for certain parameters to be correct (thought most assertions catch these). Users of the library must take care when writing applications and make sure to follow the OpenGL ES documentation, or this documentation when the implementation deviates.