#ifndef VRVIDEOPLAYER_RENDERER_H
#define VRVIDEOPLAYER_RENDERER_H

#include <pthread.h>
#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>


class Renderer {

public:
    Renderer();
    virtual ~Renderer();

    // Following methods can be called from any thread.
    // They send message to render thread which executes required actions.
    void start();
    void stop();
    void setWindow(ANativeWindow* window);

private:

    enum RenderThreadMessage {
        MSG_NONE = 0,
        MSG_WINDOW_SET,
        MSG_RENDER_LOOP_EXIT
    };

    pthread_t _threadId;
    pthread_mutex_t _mutex;
    enum RenderThreadMessage _msg;

    // android window, supported by NDK r5 and newer
    ANativeWindow* _window;

    EGLDisplay _display;
    EGLSurface _surface;
    EGLContext _context;
    GLfloat _angle;

    // RenderLoop is called in a rendering thread started in start() method
    // It creates rendering context and renders scene until stop() is called
    void renderLoop();

    bool initialize();
    void destroy();

    void drawFrame();

    // Helper method for starting the thread
    static void* threadStartCallback(void *myself);

};

#endif //VRVIDEOPLAYER_RENDERER_H
