package cz.mormegil.vrvideoplayer

import android.graphics.BitmapFactory
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import cz.mormegil.vrvideoplayer.databinding.ActivityMainBinding
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class MainActivity : AppCompatActivity() {
    companion object {
        const val TAG = "VRVideoPlayer"
    }

    private lateinit var binding: ActivityMainBinding
    private lateinit var glView: GLSurfaceView

    // Opaque native pointer to the native CardboardApp instance.
    // This object is owned by this instance and passed to the native methods.
    private var nativeApp: Long = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        Log.d(TAG, "onCreate()")

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        glView = binding.surfaceview
        glView.setEGLContextClientVersion(2)
        val renderer = Renderer()
        glView.setRenderer(renderer)
        glView.renderMode = GLSurfaceView.RENDERMODE_CONTINUOUSLY

        nativeApp = NativeLibrary.nativeInit(assets)

        glView.setOnClickListener {
            val toast = Toast.makeText(
                this,
                "This demo combines Java UI and native EGL + OpenGL renderer",
                Toast.LENGTH_LONG
            )
            toast.show();
        }
    }

    override fun onResume() {
        super.onResume()
        Log.d(TAG, "onResume()")
        glView.onResume()
        NativeLibrary.nativeOnResume(nativeApp)
    }

    override fun onPause() {
        super.onPause()
        Log.d(TAG, "onPause()")
        NativeLibrary.nativeOnPause(nativeApp)
        glView.onPause()
    }

    override fun onDestroy() {
        super.onDestroy()
        Log.d(TAG, "onDestroy()")
        NativeLibrary.nativeOnDestroy(nativeApp)
        nativeApp = 0
    }

    private inner class Renderer : GLSurfaceView.Renderer {
        override fun onSurfaceCreated(gl10: GL10?, config: EGLConfig?) {
            NativeLibrary.nativeOnSurfaceCreated(nativeApp);
        }

        override fun onSurfaceChanged(gl10: GL10?, width: Int, height: Int) {
            NativeLibrary.nativeSetScreenParams(nativeApp, width, height);
        }

        override fun onDrawFrame(gl10: GL10?) {
            NativeLibrary.nativeDrawFrame(nativeApp);
        }
    }
}
