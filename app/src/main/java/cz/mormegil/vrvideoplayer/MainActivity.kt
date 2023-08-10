package cz.mormegil.vrvideoplayer

import android.opengl.GLSurfaceView
import android.os.Bundle
import android.util.Log
import android.view.View
import android.view.WindowManager
import android.widget.RelativeLayout
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
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
    private lateinit var uiAlignmentMarker: RelativeLayout

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

        uiAlignmentMarker = binding.uiAlignmentMarker
        // TODO: Show/hide alignment marker according to output mode

        glView = binding.surfaceView
        glView.setEGLContextClientVersion(2)
        val renderer = Renderer()
        glView.setRenderer(renderer)
        glView.renderMode = GLSurfaceView.RENDERMODE_CONTINUOUSLY
        glView.setOnClickListener {
            videoTexturePlayer.seek(5000)
        }

        videoTexturePlayer = VideoTexturePlayer(this, videoUri)

        nativeApp = NativeLibrary.nativeInit(this, assets, videoTexturePlayer)

        WindowCompat.setDecorFitsSystemWindows(window, false)
        WindowInsetsControllerCompat(window, binding.root).let { controller ->
            controller.hide(WindowInsetsCompat.Type.systemBars())
            controller.systemBarsBehavior =
                WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
        }

        // Forces screen to max brightness.
        val layout = window.attributes
        layout.screenBrightness = 1f
        window.attributes = layout

        // Prevents screen from dimming/locking.
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
    }

    fun closePlayer(view: View) {
        finish()
    }

    fun showSettings(view: View) {
        NativeLibrary.nativeScanCardboardQr(nativeApp);
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
