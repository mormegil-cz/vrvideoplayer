package cz.mormegil.vrvideoplayer

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
        private const val TAG = "VRVideoPlayer"
    }

    private lateinit var binding: ActivityMainBinding
    private lateinit var glView: GLSurfaceView
    private lateinit var videoTexturePlayer: VideoTexturePlayer

    private var nativeApp: Long = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        Log.d(TAG, "onCreate()")

        val videoUri = intent.data
        if (videoUri == null) {
            // ? should not happen
            Log.w(TAG, "No URI for intent")
            finish()
            return
        }

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        glView = binding.surfaceview
        glView.setEGLContextClientVersion(2)
        val renderer = Renderer()
        glView.setRenderer(renderer)
        glView.renderMode = GLSurfaceView.RENDERMODE_CONTINUOUSLY
        glView.setOnClickListener {
            val toast = Toast.makeText(
                this,
                "This demo combines Java UI and native EGL + OpenGL renderer",
                Toast.LENGTH_LONG
            )
            toast.show()
        }

        videoTexturePlayer = VideoTexturePlayer(this, videoUri!!)

        nativeApp = NativeLibrary.nativeInit(this, assets, videoTexturePlayer)
    }

    private fun doResume() {
        glView.onResume()
        NativeLibrary.nativeOnResume(nativeApp)
        videoTexturePlayer.onResume()
    }

    override fun onResume() {
        super.onResume()
        Log.d(TAG, "onResume()")
        doResume()
    }

    override fun onPause() {
        super.onPause()
        Log.d(TAG, "onPause()")
        videoTexturePlayer.onPause()
        NativeLibrary.nativeOnPause(nativeApp)
        glView.onPause()
    }

    override fun onDestroy() {
        super.onDestroy()
        Log.d(TAG, "onDestroy()")
        NativeLibrary.nativeOnDestroy(nativeApp)
        nativeApp = 0
        videoTexturePlayer.onDestroy()
    }

    private inner class Renderer : GLSurfaceView.Renderer {
        override fun onSurfaceCreated(gl10: GL10?, config: EGLConfig?) {
            NativeLibrary.nativeOnSurfaceCreated(nativeApp)
        }

        override fun onSurfaceChanged(gl10: GL10?, width: Int, height: Int) {
            NativeLibrary.nativeSetScreenParams(nativeApp, width, height)
        }

        override fun onDrawFrame(gl10: GL10?) {
            videoTexturePlayer.updateIfNeeded()
            NativeLibrary.nativeDrawFrame(nativeApp)
        }
    }
}
