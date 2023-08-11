package cz.mormegil.vrvideoplayer

import android.opengl.GLSurfaceView
import android.os.Bundle
import android.util.Log
import android.view.MenuInflater
import android.view.MenuItem
import android.view.View
import android.view.WindowManager
import android.widget.RelativeLayout
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.PopupMenu
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import cz.mormegil.vrvideoplayer.databinding.ActivityMainBinding
import java.lang.IllegalArgumentException
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
    private var inputLayout: InputLayout = InputLayout.Mono
    private var inputMode: InputMode = InputMode.PlainFov
    private var outputMode: OutputMode = OutputMode.MonoLeft

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
        val popup = PopupMenu(this, view)
        val inflater: MenuInflater = popup.menuInflater
        inflater.inflate(R.menu.settings_menu, popup.menu)
        popup.setOnMenuItemClickListener { item: MenuItem ->
            when (item.itemId) {
                R.id.switch_viewer -> {
                    NativeLibrary.nativeScanCardboardQr(nativeApp)
                    return@setOnMenuItemClickListener true
                }

                R.id.input_layout_mono -> {
                    setInputLayout(InputLayout.Mono, item)
                    return@setOnMenuItemClickListener true
                }

                R.id.input_layout_stereo_horiz -> {
                    setInputLayout(InputLayout.StereoHoriz, item)
                    return@setOnMenuItemClickListener true
                }

                R.id.input_layout_stereo_vert -> {
                    setInputLayout(InputLayout.StereoVert, item)
                    return@setOnMenuItemClickListener true
                }

                R.id.input_mode_plain_fov -> {
                    setInputMode(InputMode.PlainFov, item)
                    return@setOnMenuItemClickListener true
                }

                R.id.input_mode_equirect_180 -> {
                    setInputMode(InputMode.Equirect180, item)
                    return@setOnMenuItemClickListener true
                }

                R.id.input_mode_equirect_360 -> {
                    setInputMode(InputMode.Equirect360, item)
                    return@setOnMenuItemClickListener true
                }

                R.id.input_mode_panorama_180 -> {
                    setInputMode(InputMode.Panorama180, item)
                    return@setOnMenuItemClickListener true
                }

                R.id.input_mode_panorama_360 -> {
                    setInputMode(InputMode.Panorama360, item)
                    return@setOnMenuItemClickListener true
                }

                R.id.output_mode_mono_left_eye -> {
                    setOutputMode(OutputMode.MonoLeft, item)
                    return@setOnMenuItemClickListener true
                }

                R.id.output_mode_mono_right_eye -> {
                    setOutputMode(OutputMode.MonoRight, item)
                    return@setOnMenuItemClickListener true
                }

                R.id.output_mode_cardboard -> {
                    setOutputMode(OutputMode.CardboardStereo, item)
                    return@setOnMenuItemClickListener true
                }

                else -> {
                    return@setOnMenuItemClickListener false
                }
            }
        }
        popup.show()
    }

    private fun setInputLayout(newLayout: InputLayout, menuItem: MenuItem) {
        if (newLayout != inputLayout) {
            inputLayout = newLayout;
            NativeLibrary.nativeSetOptions(nativeApp, inputLayout.ordinal, inputMode.ordinal, outputMode.ordinal);
            menuItem.isChecked = true
        }
    }

    private fun setInputMode(newMode: InputMode, menuItem: MenuItem) {
        if (newMode != inputMode) {
            inputMode = newMode;
            NativeLibrary.nativeSetOptions(nativeApp, inputLayout.ordinal, inputMode.ordinal, outputMode.ordinal);
            menuItem.isChecked = true
        }
    }

    private fun setOutputMode(newMode: OutputMode, menuItem: MenuItem) {
        if (newMode != outputMode) {
            outputMode = newMode;
            NativeLibrary.nativeSetOptions(nativeApp, inputLayout.ordinal, inputMode.ordinal, outputMode.ordinal);
            menuItem.isChecked = true
        }
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
