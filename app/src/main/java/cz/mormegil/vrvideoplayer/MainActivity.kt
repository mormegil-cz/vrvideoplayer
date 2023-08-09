package cz.mormegil.vrvideoplayer

import android.content.Intent
import android.net.Uri
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.activity.result.PickVisualMediaRequest
import androidx.activity.result.contract.ActivityResultContracts
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

    private var ready = false
    private var resume = false

    private val videoGalleryChooser =
        registerForActivityResult((ActivityResultContracts.PickVisualMedia()), ::initWithVideo)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        Log.d(TAG, "onCreate()")

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        glView = binding.surfaceview
        glView.setOnClickListener {
            val toast = Toast.makeText(
                this,
                "This demo combines Java UI and native EGL + OpenGL renderer",
                Toast.LENGTH_LONG
            )
            toast.show();
        }

        when (intent.action) {
            Intent.ACTION_VIEW -> {
                val viewUri = intent.data;
                if (viewUri != null) {
                    initWithVideo(viewUri);
                } else {
                    chooseVideoFromGallery();
                }
            }

            // Intent.ACTION_MAIN ->
            else -> {
                chooseVideoFromGallery();
            }
        }
    }

    private fun chooseVideoFromGallery() {
        videoGalleryChooser.launch(PickVisualMediaRequest(ActivityResultContracts.PickVisualMedia.VideoOnly))
    }

    private fun initWithVideo(videoUri: Uri?) {
        if (videoUri == null) {
            // bah!
            finish()
        }

        glView.setEGLContextClientVersion(2)
        val renderer = Renderer()
        glView.setRenderer(renderer)
        glView.renderMode = GLSurfaceView.RENDERMODE_CONTINUOUSLY

        videoTexturePlayer = VideoTexturePlayer(this, videoUri!!)

        nativeApp = NativeLibrary.nativeInit(this, assets, videoTexturePlayer)

        ready = true

        if (resume) {
            resume = false
            doResume()
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
        if (!ready) {
            resume = true
            return
        }
        doResume()
    }

    override fun onPause() {
        super.onPause()
        Log.d(TAG, "onPause()")
        if (!ready) {
            resume = false
            return
        }
        videoTexturePlayer.onPause()
        NativeLibrary.nativeOnPause(nativeApp)
        glView.onPause()
    }

    override fun onDestroy() {
        super.onDestroy()
        if (!ready) {
            return
        }
        Log.d(TAG, "onDestroy()")
        NativeLibrary.nativeOnDestroy(nativeApp)
        nativeApp = 0
        videoTexturePlayer.onDestroy()
    }

    private inner class Renderer : GLSurfaceView.Renderer {
        override fun onSurfaceCreated(gl10: GL10?, config: EGLConfig?) {
            NativeLibrary.nativeOnSurfaceCreated(nativeApp);
        }

        override fun onSurfaceChanged(gl10: GL10?, width: Int, height: Int) {
            NativeLibrary.nativeSetScreenParams(nativeApp, width, height);
        }

        override fun onDrawFrame(gl10: GL10?) {
            videoTexturePlayer.updateIfNeeded()
            NativeLibrary.nativeDrawFrame(nativeApp);
        }
    }
}
