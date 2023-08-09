package cz.mormegil.vrvideoplayer

import android.content.Context
import android.graphics.SurfaceTexture
import android.graphics.SurfaceTexture.OnFrameAvailableListener
import android.media.AudioAttributes
import android.media.MediaPlayer
import android.net.Uri
import android.opengl.GLES20
import android.util.Log
import android.view.Surface
import java.util.concurrent.atomic.AtomicBoolean


class VideoTexturePlayer(private val context: Context, private val videoSourceUri: Uri) :
    OnFrameAvailableListener {
    companion object {
        private const val TAG = "VRVideoPlayerV"
        private fun checkGLErrors(where: String) {
            var error: Int
            while (GLES20.glGetError().also { error = it } != GLES20.GL_NO_ERROR) {
                Log.e(TAG, "GLError detected at $where: $error")
            }
        }
    }

    private var surfaceTexture: SurfaceTexture? = null
    private var mediaPlayer: MediaPlayer? = null
    private val frameAvailable: AtomicBoolean = AtomicBoolean(false)

    fun initializePlayback(texName: Int) {
        cleanup()

        val surfaceTexture = SurfaceTexture(texName)
        this.surfaceTexture = surfaceTexture
        surfaceTexture.setOnFrameAvailableListener(this);
        checkGLErrors("new SurfaceTexture")

        val mediaPlayer = MediaPlayer()
        this.mediaPlayer = mediaPlayer
        mediaPlayer.setDataSource(context, videoSourceUri)

        val surface = Surface(surfaceTexture)
        checkGLErrors("new Surface")
        mediaPlayer.setSurface(surface)
        surface.release()
        checkGLErrors("MediaPlayer setSurface")

        mediaPlayer.prepare()
        checkGLErrors("MediaPlayer prepare")
        /*
    mediaPlayer.setOnBufferingUpdateListener(this);
    mediaPlayer.setOnCompletionListener(this);
    mediaPlayer.setOnPreparedListener(this);
    mediaPlayer.setOnVideoSizeChangedListener(this);
     */
        /*
    mediaPlayer.setOnBufferingUpdateListener(this);
    mediaPlayer.setOnCompletionListener(this);
    mediaPlayer.setOnPreparedListener(this);
    mediaPlayer.setOnVideoSizeChangedListener(this);
     */
        mediaPlayer.setAudioAttributes(
            AudioAttributes.Builder()
                .setContentType(AudioAttributes.CONTENT_TYPE_MOVIE)
                .build()
        )
        checkGLErrors("mediaPlayer set")
        mediaPlayer.start()
        checkGLErrors("mediaPlayer start")

        Log.d(TAG, "VideoTexturePlayer initialized with $videoSourceUri")
    }

    private fun cleanup() {
        val mediaPlayer = this.mediaPlayer
        this.mediaPlayer = null
        if (mediaPlayer != null) {
            mediaPlayer.stop()
            mediaPlayer.release()
        }
        val surfaceTexture = this.surfaceTexture
        this.surfaceTexture = null
        surfaceTexture?.release()
        checkGLErrors("cleanup")

        Log.d(TAG, "VideoTexturePlayer cleaned up")
    }

    fun onPause() {
        mediaPlayer?.pause()
        Log.d(TAG, "onPause")
    }

    fun onResume() {
        mediaPlayer?.start()
        Log.d(TAG, "onResume")
    }

    fun onDestroy() {
        cleanup()
        Log.d(TAG, "onDestroy")
    }

    override fun onFrameAvailable(tex: SurfaceTexture?) {
        frameAvailable.set(true)
    }

    fun updateIfNeeded() {
        if (frameAvailable.getAndSet(false)) {
            surfaceTexture?.updateTexImage()
        }
    }
}
