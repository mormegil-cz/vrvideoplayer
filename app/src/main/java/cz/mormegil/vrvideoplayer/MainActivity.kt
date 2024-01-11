package cz.mormegil.vrvideoplayer

import android.annotation.SuppressLint
import android.graphics.PointF
import android.media.MediaPlayer
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.util.Log
import android.view.MenuInflater
import android.view.MenuItem
import android.view.MotionEvent
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
import kotlin.math.roundToInt


class MainActivity : AppCompatActivity(), MediaPlayer.OnVideoSizeChangedListener {
    companion object {
        private const val TAG = "VRVideoPlayer"
    }

    private lateinit var binding: ActivityMainBinding
    private lateinit var glView: GLSurfaceView
    private lateinit var videoTexturePlayer: VideoTexturePlayer

    private var nativeApp: Long = 0
    private var inputLayout: InputLayout = InputLayout.Mono
    private var inputMode: InputMode = InputMode.PlainFov
    private var outputMode: OutputMode = OutputMode.MonoLeft

    private var lastTouchCoordinates = arrayOf(1.0f, 0.0f);

    @SuppressLint("ClickableViewAccessibility") // VR video really does not support accessibility
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

        glView = binding.surfaceView
        glView.setEGLContextClientVersion(2)
        val renderer = Renderer()
        glView.setRenderer(renderer)
        glView.renderMode = GLSurfaceView.RENDERMODE_CONTINUOUSLY
        glView.setOnTouchListener { _, event ->
            // save the X,Y coordinates
            if (event.actionMasked == MotionEvent.ACTION_DOWN) {
                lastTouchCoordinates[0] = event.x;
                lastTouchCoordinates[1] = event.y;
            }

            // let the touch event pass on to whoever needs it
            return@setOnTouchListener false
        }
        glView.setOnClickListener {
            val x = lastTouchCoordinates[0];
            val halfWidth = glView.width.coerceAtLeast(1) * 0.5f
            val relX = (x - halfWidth) / halfWidth

            videoTexturePlayer.seek((10000 * relX).roundToInt())
            NativeLibrary.nativeShowProgressBar(nativeApp)
        }

        videoTexturePlayer = VideoTexturePlayer(this, videoUri, this)

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

                R.id.input_layout_anaglyph_red_cyan -> {
                    setInputLayout(InputLayout.AnaglyphRedCyan, item)
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
            NativeLibrary.nativeSetOptions(
                nativeApp,
                inputLayout.ordinal,
                inputMode.ordinal,
                outputMode.ordinal
            );
            menuItem.isChecked = true
        }
    }

    private fun setInputMode(newMode: InputMode, menuItem: MenuItem) {
        if (newMode != inputMode) {
            inputMode = newMode;
            NativeLibrary.nativeSetOptions(
                nativeApp,
                inputLayout.ordinal,
                inputMode.ordinal,
                outputMode.ordinal
            );
            menuItem.isChecked = true
        }
    }

    private fun setOutputMode(newMode: OutputMode, menuItem: MenuItem) {
        if (newMode != outputMode) {
            outputMode = newMode;
            NativeLibrary.nativeSetOptions(
                nativeApp,
                inputLayout.ordinal,
                inputMode.ordinal,
                outputMode.ordinal
            );
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

    override fun onVideoSizeChanged(mp: MediaPlayer?, width: Int, height: Int) {
        Log.d(TAG, "onVideoSizeChanged()")
        NativeLibrary.nativeOnVideoSizeChanged(nativeApp, width, height);
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
            NativeLibrary.nativeDrawFrame(nativeApp, videoTexturePlayer.getVideoPosition())
        }
    }
}
