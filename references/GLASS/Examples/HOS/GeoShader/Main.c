#include <GLES/gl2.h>
#include <kazmath/kazmath.h>

#include "GeoShader_shaders_shbin.h"

typedef struct {
    float x, y, z;
} Position;

typedef struct {
    float r, g, b, a;
} Color;

typedef struct {
    Position pos;
    Color clr;
} Vertex;

static const Vertex g_VertexList[3] = {
    {
        { 200.0f, 200.0f, 0.5f }, // Top
        { 1.0f, 0.0f, 0.0f, 1.0f }, // Red
    },
    {
        { 100.0f, 40.0f, 0.5f }, // Left
        { 0.0f, 1.0f, 0.0f, 1.0f }, // Green
    },
    {
        { 300.0f, 40.0f, 0.5f },  // Right
        { 0.0f, 0.0f, 1.0f, 1.0f }, // Blue
    },
};

#define NUM_VERTICES sizeof(g_VertexList)/sizeof(Vertex)

static GLint g_ProjLoc;
static kmMat4 g_ProjMtx;

static GLint sceneInit(u16 screenWidth, u16 screenHeight) {
    // Set default state.
    glViewport(0, 0, screenWidth, screenHeight);
    glClearColor(0.4f, 0.68f, 0.84f, 1.0f);

    // Load the shaders, create a shader program and bind them.
    GLuint shads[2];
    GLuint prog = glCreateProgram();

    shads[0] = glCreateShader(GL_VERTEX_SHADER);
    shads[1] = glCreateShader(GL_GEOMETRY_SHADER_PICA);

    glShaderBinary(2, shads, GL_SHADER_BINARY_PICA, GeoShader_shaders_shbin, GeoShader_shaders_shbin_size);

    for (size_t i = 0; i < sizeof(shads)/sizeof(GLuint); ++i) {
        glAttachShader(prog, shads[i]);
        glDeleteShader(shads[i]);
    }

    glLinkProgram(prog);
    glUseProgram(prog);
    glDeleteProgram(prog);

    // Set geometry shader stride.
    glProgramGeometryStridePICA(prog, 6);

    // Get the location of the uniforms.
    g_ProjLoc = glGetUniformLocation(prog, "projection");

    // Create the VBO (vertex buffer object).
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_VertexList), g_VertexList, GL_STATIC_DRAW);

    // Configure attributes for use with the vertex shader.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), NULL); // v0 = position
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, clr)); // v1 = color
    glEnableVertexAttribArray(1);

    // Compute the projection matrix.
    kmMat4 mtx;
    kmMat4OrthoTilt(&mtx, 0.0f, 400.0f, 0.0f, 240.0f, 0.0f, 1.0f, true);
    kmMat4Transpose(&g_ProjMtx, &mtx);
}

static void sceneRender(void) {
    // Update the uniforms.
    glUniformMatrix4fv(g_ProjLoc, 1, GL_FALSE, g_ProjMtx.mat);

    // Draw the VBO.
    glDrawArrays(GL_GEOMETRY_PRIMITIVE_PICA, 0, NUM_VERTICES);
}

int main() {
    // Initialize graphics.
    gfxInitDefault();
    kygxInit();

    // Create context.
    GLASSCtx ctx = glassCreateDefaultContext(GLASS_VERSION_ES_2);
    glassBindContext(ctx);

    // Initialize the render target.
    u16 screenWidth = 0;
    u16 screenHeight = 0;
    glassGetScreenFramebuffer(ctx, &screenWidth, &screenHeight, NULL);

    GLuint fb;
    GLuint rb;

    glGenFramebuffers(1, &fb);
    glGenRenderbuffers(1, &rb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glBindRenderbuffer(GL_RENDERBUFFER, rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, screenWidth, screenHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb);

    // Initialize the scene.
    GLint vbo = sceneInit(screenWidth, screenHeight);

    // Main loop.
    while (aptMainLoop()) {
        hidScanInput();

        // Respond to user input.
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START)
            break; // break in order to return to hbmenu.

        // Render the scene.
        glClear(GL_COLOR_BUFFER_BIT);
        sceneRender();
        glassSwapBuffers();
    }

    // Deinitialize graphics.
    glDeleteBuffers(1, &vbo);
    glDeleteRenderbuffers(1, &rb);
    glDeleteFramebuffers(1, &fb);

    glassDestroyContext(ctx);

    kygxExit();
    gfxExit();
    return 0;
}