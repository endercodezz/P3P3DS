#include <GLES/gl2.h>
#include <kazmath/kazmath.h>

#include <drivers/gfx.h>
#include <arm11/power.h>
#include <arm11/console.h>
#include <arm11/fmt.h>
#include <arm11/drivers/lcd.h>
#include <arm11/drivers/hid.h>
#include <arm11/drivers/mcu.h>
#include <arm11/drivers/i2c.h>
#include <arm11/drivers/gpio.h>

#include "Lenny.h"
#include "Lenny_vshader_shbin.h"

static GLint g_ProjLoc;
static GLint g_ModelViewLoc;

static kmMat4 g_BaseModelView;

static bool isN3DModel(McuSysModel model) {
    return model == SYS_MODEL_N3DS || model == SYS_MODEL_N3DS_XL;
}

static bool is3DModel(McuSysModel model) {
    return model == SYS_MODEL_3DS || model == SYS_MODEL_3DS_XL || isN3DModel(model);
}

// Many thanks to TuxSH.
static inline void expanderInit(void) {
    GPIO_config(GPIO_3_11, GPIO_OUTPUT);
    GPIO_write(GPIO_3_11, 1);

    const u16 zero = 0;
    I2C_writeArray(I2C_DEV_IO_EXP, 6, &zero, sizeof(u16));
    I2C_writeArray(I2C_DEV_IO_EXP, 2, &zero, sizeof(u16));
}

static inline void expanderExit(void) {
    GPIO_config(GPIO_3_11, GPIO_OUTPUT);
    GPIO_write(GPIO_3_11, 0);
}

static inline void setExpanderPresets(u16 polarityHigh, u16 polarityLow) {
    I2C_writeArray(I2C_DEV_IO_EXP, 2, &polarityHigh, sizeof(u16));
    I2C_writeArray(I2C_DEV_IO_EXP, 2, &polarityLow, sizeof(u16));
}

static void enable3DEffect(McuSysModel model) {
    if (is3DModel(model))
        getLcdRegs()->parallax_cnt = (PARALLAX_CNT_PWM0_EN | PARALLAX_CNT_PWM1_EN);

    if (isN3DModel(model)) {
        expanderInit();
        setExpanderPresets(0x11F0, 0xE0F); // O3DS (SS3D disabled).
    }

    GFX_setFormat(GFX_BGR8, GFX_BGR565, GFX_TOP_3D);
    // TODO: luminance.
}

static void disable3DEffect(McuSysModel model) {
    if (isN3DModel(model))
        expanderExit();

    if (is3DModel(model))
        getLcdRegs()->parallax_cnt = 0;

    GFX_setFormat(GFX_BGR8, GFX_BGR565, GFX_TOP_2D);
    GFX_setLcdLuminance(80);
}

static GLuint sceneInit(void) {
    // Set default state.
    glViewport(0, 0, 400, 240);
    glClearColor(0.4f, 0.68f, 0.84f, 1.0f);
    glClearDepthf(0.0f);

    // Load the vertex shader, create a shader program and bind it.
    GLuint prog = glCreateProgram();
    GLuint shad = glCreateShader(GL_VERTEX_SHADER);
    glShaderBinary(1, &shad, GL_SHADER_BINARY_PICA, Lenny_vshader_shbin, Lenny_vshader_shbin_size);
    glAttachShader(prog, shad);
    glDeleteShader(shad);
    glLinkProgram(prog);
    glUseProgram(prog);
    glDeleteProgram(prog);

     // Get the location of the uniforms.
    g_ProjLoc = glGetUniformLocation(prog, "projection");
    g_ModelViewLoc = glGetUniformLocation(prog, "modelView");

    // Create and bind the VBO (vertex buffer object).
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_VertexList), g_VertexList, GL_STATIC_DRAW);

    // Configure attributes.
    const GLint inpos = glGetAttribLocation(prog, "inpos");
    glVertexAttribPointer(inpos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(inpos);

    const GLint innrm = glGetAttribLocation(prog, "innrm");
    glVertexAttribPointer(innrm, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nx));
    glEnableVertexAttribArray(innrm);

    // Compute the base model view matrix (avoid computation in the rendering step).
    kmVec4 objPos;
    kmVec4Fill(&objPos, 0.0f, 0.0f, -3.0f, 1.0f);

    kmMat4 tmp;
    kmMat4 tmp2;

    kmMat4Identity(&tmp);
    kmMat4Translation(&tmp2, objPos.x, objPos.y, objPos.z);
    kmMat4Multiply(&g_BaseModelView, &tmp, &tmp2);

    memcpy(&tmp, &g_BaseModelView, sizeof(kmMat4));
    kmMat4Scaling(&tmp2, 2.0f, 2.0f, 2.0f);
    kmMat4Multiply(&g_BaseModelView, &tmp, &tmp2);

    // TODO: light.
}

static void sceneRender(float iod, float angleX, float angleY) {
    // Compute the projection matrix.
    kmMat4 tmp;
    kmMat4 projection;
    kmMat4PerspStereoTilt(&tmp, kmDegreesToRadians(40.0f), 400.0f/240.0f, 0.01f, 1000.0f, iod, 2.0f, false);
    kmMat4Transpose(&projection, &tmp);

    // Compute the model matrix.
    kmMat4 modelView;
    kmMat4RotationY(&modelView, kmRevolutionsToRadians(angleY));
    kmMat4Multiply(&tmp, &g_BaseModelView, &modelView);
    kmMat4Transpose(&modelView, &tmp);

    // Update the uniforms.
    glUniformMatrix4fv(g_ProjLoc, 1, GL_FALSE, projection.mat);
    glUniformMatrix4fv(g_ModelViewLoc, 1, GL_FALSE, modelView.mat);

    // Draw the VBO.
    glDrawArrays(GL_TRIANGLES, 0, VERTEX_LIST_COUNT);
}

static void createFramebuffer(GLuint* fb, GLuint* rb) {
    glGenFramebuffers(1, fb);
    glGenRenderbuffers(2, rb);

    glBindFramebuffer(GL_FRAMEBUFFER, *fb);

    glBindRenderbuffer(GL_RENDERBUFFER, rb[0]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8_OES, 400, 240);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rb[0]);

    glBindRenderbuffer(GL_RENDERBUFFER, rb[1]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, 400, 240);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb[1]);
}

int main() {
    // Initialize graphics.
    GFX_init(GFX_BGR8, GFX_BGR565, GFX_TOP_2D);
    GFX_setLcdLuminance(80);
    consoleInit(GFX_LCD_BOT, NULL);
    kygxInit();

    // Create context.
    GLASSCtx ctx = glassCreateDefaultContext(GLASS_VERSION_ES_2);
    glassBindContext(ctx);

    /// Initialize the render targets.
    GLuint leftFB;
    GLuint leftRBs[2];

    glassSetTargetSide(ctx, GLASS_SIDE_LEFT);
    createFramebuffer(&leftFB, leftRBs);

    GLuint rightFB;
    GLuint rightRBs[2];

    glassSetTargetSide(ctx, GLASS_SIDE_RIGHT);
    createFramebuffer(&rightFB, rightRBs);

    // Initialize the scene.
    GLuint vbo = sceneInit();

    // Main loop.
    const McuSysModel model = MCU_getSystemModel();
    bool has3D = false;

    float angleX = 0.0;
    float angleY = 0.0;

    ee_printf("Press X to control 3D.\n");
    ee_printf("3D state: DISABLED\n");

    while (true) {
        // Scan for input.
        hidScanInput();

        // Respond to user input.
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        if (kDown & KEY_START)
            break; // break in order to exit.

        // Control 3D effect.
        if (kDown & KEY_X) {
            consoleClear();
            ee_printf("Press X to control 3D.\n");

            if (!has3D) {
                enable3DEffect(model);
                ee_printf("3D state: ENABLED\n");
                has3D = true;
            } else {
                disable3DEffect(model);
                ee_printf("3D state: DISABLED\n");
                has3D = false;
            }
        }

        // Rotate the model.
        if (!(kHeld & KEY_A)) {
            angleX += 1.0f/64;
            angleY += 1.0f/256;
        }

        // Compute the interocular distance.
        const float slider = MCU_get3dSliderPosition() / 256.0f;
        const float iod = slider / 3;

        // Render left.
        glassSetTargetSide(ctx, GLASS_SIDE_LEFT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        sceneRender(-iod, angleX, angleY);

        // Render right.
        if (iod > 0.0f) {
            glassSetTargetSide(ctx, GLASS_SIDE_RIGHT);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            sceneRender(iod, angleX, angleY);
        }

        glassSwapBuffers();
    }

    // Disable 3D effect.
    disable3DEffect(model);

    // Deinitialize graphics.
    glDeleteBuffers(1, &vbo);
    
    glDeleteRenderbuffers(2, rightRBs);
    glDeleteFramebuffers(1, &rightFB);

    glDeleteRenderbuffers(2, leftRBs);
    glDeleteFramebuffers(1, &leftFB);

    glassDestroyContext(ctx);

    kygxExit();
    GFX_deinit();
    power_off();
    return 0;
}