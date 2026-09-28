#include <GLES/gl2.h>
#include <kazmath/kazmath.h>

#include "RenderTexture_vshader_shbin.h"

typedef struct {
    float x, y, z;
} Position;

typedef struct {
    float r, g, b, a;
} Color;

static const Position g_VertexList[3] = {
    { 200.0f, 200.0f, 0.5f }, // Top
    { 100.0f, 40.0f, 0.5f },  // Left
    { 300.0f, 40.0f, 0.5f },  // Right
};

static const Color g_ColorList[3] = {
    { 1.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 1.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 1.0f, 1.0f },
};

static GLint g_ProjLoc;
static kmMat4 g_ProjMtx;

static void sceneInit(u16 width, u16 height, GLuint* vbos) {
    // Set default state.
    glViewport(0, 0, width, height);
    glClearColor(0.4f, 0.68f, 0.84f, 1.0f);

    // Load the vertex shader, create a shader program and bind it.
    GLuint prog = glCreateProgram();
    GLuint shad = glCreateShader(GL_VERTEX_SHADER);
    glShaderBinary(1, &shad, GL_SHADER_BINARY_PICA, RenderTexture_vshader_shbin, RenderTexture_vshader_shbin_size);
    glAttachShader(prog, shad);
    glDeleteShader(shad);
    glLinkProgram(prog);
    glUseProgram(prog);
    glDeleteProgram(prog);

    // Get the location of the uniforms.
    g_ProjLoc = glGetUniformLocation(prog, "projection");

    // Create the VBOs (vertex buffer objects).
    glGenBuffers(2, vbos);

    // Configure position attribute.
    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_VertexList), g_VertexList, GL_STATIC_DRAW);

    const GLint inpos = glGetAttribLocation(prog, "inpos");
    glVertexAttribPointer(inpos, 3, GL_FLOAT, GL_FALSE, sizeof(Position), NULL);
    glEnableVertexAttribArray(inpos);

    // Configure color attribute.
    glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_ColorList), g_ColorList, GL_STATIC_DRAW);

    const GLint inclr = glGetAttribLocation(prog, "inclr");
    glVertexAttribPointer(inclr, 4, GL_FLOAT, GL_FALSE, sizeof(Color), NULL);
    glEnableVertexAttribArray(inclr);

    // Compute the projection matrix.
    kmMat4 mtx;
    kmMat4OrthoTilt(&mtx, 0.0f, 400.0f, 0.0f, 240.0f, 0.0f, 1.0f, true);
    kmMat4Transpose(&g_ProjMtx, &mtx);
}

static void sceneRender(void) {
    // Update the uniforms.
    glUniformMatrix4fv(g_ProjLoc, 1, GL_FALSE, g_ProjMtx.mat);

    // Draw the VBO.
    glDrawArrays(GL_TRIANGLES, 0, 3);
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
    GLuint tex;

    glGenFramebuffers(1, &fb);
    glGenTextures(1, &tex);

    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexVRAMPICA(GL_TRUE); // Required for glClear.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    // Initialize the scene.
    GLuint vbos[2];
    sceneInit(screenWidth, screenHeight, vbos);

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
    glDeleteBuffers(2, vbos);
    glDeleteTextures(1, &tex);
    glDeleteFramebuffers(1, &fb);

    glassDestroyContext(ctx);

    kygxExit();
    gfxExit();
    return 0;
}