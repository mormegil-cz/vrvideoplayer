#include <cstdint>
#include <unistd.h>
#include <pthread.h>
#include <android/native_window.h>
#include <EGL/egl.h>
#include <GLES/gl.h>

#include "Renderer.h"
#include "logger.h"

#define LOG_TAG "VrVideoPlayerR"

static GLint vertices[][3] = {
        {-0x10000, -0x10000, -0x10000},
        {0x10000,  -0x10000, -0x10000},
        {0x10000,  0x10000,  -0x10000},
        {-0x10000, 0x10000,  -0x10000},
        {-0x10000, -0x10000, 0x10000},
        {0x10000,  -0x10000, 0x10000},
        {0x10000,  0x10000,  0x10000},
        {-0x10000, 0x10000,  0x10000}
};

static GLint colors[][4] = {
        {0x00000, 0x00000, 0x00000, 0x10000},
        {0x10000, 0x00000, 0x00000, 0x10000},
        {0x10000, 0x10000, 0x00000, 0x10000},
        {0x00000, 0x10000, 0x00000, 0x10000},
        {0x00000, 0x00000, 0x10000, 0x10000},
        {0x10000, 0x00000, 0x10000, 0x10000},
        {0x10000, 0x10000, 0x10000, 0x10000},
        {0x00000, 0x10000, 0x10000, 0x10000}
};

GLubyte indices[] = {
        0, 4, 5, 0, 5, 1,
        1, 5, 6, 1, 6, 2,
        2, 6, 7, 2, 7, 3,
        3, 7, 4, 3, 4, 0,
        4, 7, 6, 4, 6, 5,
        3, 0, 1, 3, 1, 2
};


Renderer::Renderer()
        : _msg(MSG_NONE),
          _display(nullptr),
          _surface(nullptr),
          _context(nullptr),
          _angle(0),
          _threadId(0),
          _window(nullptr),
          _mutex({}) {
    LOG_DEBUG("Renderer instance created");
    pthread_mutex_init(&_mutex, nullptr);
}

Renderer::~Renderer() {
    LOG_DEBUG("Renderer instance destroyed");
    pthread_mutex_destroy(&_mutex);
}

void Renderer::start() {
    LOG_DEBUG("Creating renderer thread");
    pthread_create(&_threadId, nullptr, threadStartCallback, this);
}

void Renderer::stop() {
    LOG_DEBUG("Stopping renderer thread");

    // send message to render thread to stop rendering
    pthread_mutex_lock(&_mutex);
    _msg = MSG_RENDER_LOOP_EXIT;
    pthread_mutex_unlock(&_mutex);

    pthread_join(_threadId, nullptr);

    LOG_DEBUG("Renderer thread stopped");
}

void Renderer::setWindow(ANativeWindow *window) {
    // notify render thread that window has changed
    pthread_mutex_lock(&_mutex);
    if (_window) {
        LOG_INFO("Window already set");
        if (window == _window) {
            pthread_mutex_unlock(&_mutex);
            return;
        }
        destroy();
    }
    _msg = MSG_WINDOW_SET;
    _window = window;
    pthread_mutex_unlock(&_mutex);
}


void Renderer::renderLoop() {
    bool renderingEnabled = true;

    LOG_DEBUG("renderLoop()");
    unsigned long frames = 0L;

    while (renderingEnabled) {
        pthread_mutex_lock(&_mutex);
        // process incoming messages
        switch (_msg) {
            case MSG_WINDOW_SET:
                initialize();
                break;

            case MSG_RENDER_LOOP_EXIT:
                renderingEnabled = false;
                destroy();
                break;

            default:
                break;
        }
        _msg = MSG_NONE;
        pthread_mutex_unlock(&_mutex);

        if (_display) {
            ++frames;
            drawFrame();
            if (!eglSwapBuffers(_display, _surface)) {
                LOG_ERROR("eglSwapBuffers() returned error %d", eglGetError());
            }
        }
    }

    LOG_DEBUG("Render loop exits after %zu frames", frames);
}

bool Renderer::initialize() {
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };
    EGLDisplay display;
    EGLConfig config;
    EGLint numConfigs;
    EGLint format;
    EGLSurface surface;
    EGLContext context;
    EGLint width;
    EGLint height;
    GLfloat ratio;

    LOG_DEBUG("Initializing context");

    if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        LOG_ERROR("eglGetDisplay() returned error %d", eglGetError());
        return false;
    }
    if (!eglInitialize(display, nullptr, nullptr)) {
        LOG_ERROR("eglInitialize() returned error %d", eglGetError());
        return false;
    }

    if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs)) {
        LOG_ERROR("eglChooseConfig() returned error %d", eglGetError());
        destroy();
        return false;
    }

    if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)) {
        LOG_ERROR("eglGetConfigAttrib() returned error %d", eglGetError());
        destroy();
        return false;
    }

    ANativeWindow_setBuffersGeometry(_window, 0, 0, format);

    if (!(surface = eglCreateWindowSurface(display, config, _window, 0))) {
        LOG_ERROR("eglCreateWindowSurface() returned error %d", eglGetError());
        destroy();
        return false;
    }

    if (!(context = eglCreateContext(display, config, nullptr, nullptr))) {
        LOG_ERROR("eglCreateContext() returned error %d", eglGetError());
        destroy();
        return false;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        LOG_ERROR("eglMakeCurrent() returned error %d", eglGetError());
        destroy();
        return false;
    }

    if (!eglQuerySurface(display, surface, EGL_WIDTH, &width) ||
        !eglQuerySurface(display, surface, EGL_HEIGHT, &height)) {
        LOG_ERROR("eglQuerySurface() returned error %d", eglGetError());
        destroy();
        return false;
    }

    _display = display;
    _surface = surface;
    _context = context;

    glDisable(GL_DITHER);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glClearColor(0, 0, 0, 0);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, width, height);

    ratio = (GLfloat) width / (GLfloat) height;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustumf(-ratio, ratio, -1, 1, 1, 10);

    return true;
}

void Renderer::destroy() {
    LOG_DEBUG("Destroying context");

    eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(_display, _context);
    eglDestroySurface(_display, _surface);
    eglTerminate(_display);

    _display = EGL_NO_DISPLAY;
    _surface = EGL_NO_SURFACE;
    _context = EGL_NO_CONTEXT;
}

void Renderer::drawFrame() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -3.0f);
    glRotatef(_angle, 0, 1, 0);
    glRotatef(_angle * 0.25f, 1, 0, 0);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glFrontFace(GL_CW);
    glVertexPointer(3, GL_FIXED, 0, vertices);
    glColorPointer(4, GL_FIXED, 0, colors);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, indices);

    _angle += 1.2f;
}

void *Renderer::threadStartCallback(void *myself) {
    auto *renderer = (Renderer *) myself;

    renderer->renderLoop();
    pthread_exit(nullptr);
}
