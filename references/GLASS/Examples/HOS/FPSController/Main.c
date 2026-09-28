#include <GLES/gl2.h>
#include <kazmath/kazmath.h>

#include "FPSController_vshader_shbin.h"

typedef struct {
    float x, y, z;
} Vertex;

static const Vertex g_VertexList[3] = {
    {    0.0f, 200.0f, 0.5f },
    { -100.0f,  40.0f, 0.5f },
    {  100.0f,  40.0f, 0.5f },
};

#define VERTEX_LIST_COUNT (sizeof(g_VertexList)/sizeof(g_VertexList[0]))

static GLint g_ProjLoc;
static kmMat4 g_Projection;

static u32 g_FPS = 0;

static GLuint sceneInit(u16 width, u16 height) {
    // Set default state.
    glViewport(0, 0, width, height);
    glClearColor(0.4f, 0.68f, 0.84f, 1.0f);

    // Load the vertex shader, create a shader program and bind it.
    GLuint prog = glCreateProgram();
    GLuint shad = glCreateShader(GL_VERTEX_SHADER);
    glShaderBinary(1, &shad, GL_SHADER_BINARY_PICA, FPSController_vshader_shbin, FPSController_vshader_shbin_size);
    glAttachShader(prog, shad);
    glDeleteShader(shad);
    glLinkProgram(prog);
    glUseProgram(prog);
    glDeleteProgram(prog);

    // Get the location of the uniforms.
    g_ProjLoc = glGetUniformLocation(prog, "projection");

    // Create the VBO (vertex buffer object).
    GLuint vbo;

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_VertexList), g_VertexList, GL_STATIC_DRAW);

    // Configure attributes.
    const GLint inpos = glGetAttribLocation(prog, "inpos");
    glVertexAttribPointer(inpos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), NULL);
    glEnableVertexAttribArray(inpos);

    const GLint inclr = glGetAttribLocation(prog, "inclr");
    glVertexAttrib3f(inclr, 1.0f, 1.0f, 1.0f);
    glEnableVertexAttribArray(inclr);

    // Compute the projection matrix.
    kmMat4 tmp;
    kmMat4OrthoTilt(&tmp, -200.0f, 200.0f, 0.0f, 240.0f, 0.0f, 1.0f, true);
    kmMat4Transpose(&g_Projection, &tmp);

    return vbo;
}

static void sceneRender(float a) {
    // Compute color.
    float b = (cosf(kmRevolutionsToRadians(a)) + 1.0f) / 2.0f;

    // Update the uniforms.
    glUniformMatrix4fv(g_ProjLoc, 1, GL_FALSE, g_Projection.mat);
    glVertexAttrib3f(1, b, b, b);

    // Draw the VBO.
    glDrawArrays(GL_TRIANGLES, 0, VERTEX_LIST_COUNT);
}

static void updateFPS(u32 fps) {
    if (fps == 0)
        fps = 1;

    if (fps > 120)
        fps = 120;

    g_FPS = fps;

    consoleClear();
    printf("FPS: %u\n", g_FPS);
}

int main() {
    // Initialize graphics.
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);
    kygxInit();

    // Create context.
    GLASSCtxParams ctxParams;
    glassGetDefaultContextParams(&ctxParams, GLASS_VERSION_ES_2);
    ctxParams.vsync = false;

    GLASSCtx ctx = glassCreateContext(&ctxParams);
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
    GLuint vbo = sceneInit(screenWidth, screenHeight);

    // Main loop.
    float count = 0.0f;
    updateFPS(60);

    while (aptMainLoop()) {
        hidScanInput();

        // Respond to user input.
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START)
            break; // break in order to return to hbmenu.

        if (kDown & KEY_UP)
            updateFPS(g_FPS + 1);

        if (kDown & KEY_DOWN)
            updateFPS(g_FPS - 1);

        if (kDown & KEY_RIGHT)
            updateFPS(g_FPS + 10);

        if (kDown & KEY_LEFT)
            updateFPS(g_FPS - 10);

        // Render the scene.
        glClear(GL_COLOR_BUFFER_BIT);
        sceneRender(count);
        glassSwapBuffers();

        svcSleepThread(1.0f/g_FPS * 1000 * 1000 * 1000);
        count += 1/128.0f * 60.0f/g_FPS;
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