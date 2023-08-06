package cz.mormegil.vrvideoplayer

import android.content.res.AssetManager
import android.graphics.SurfaceTexture
import android.media.AudioAttributes
import android.media.MediaPlayer
import android.opengl.GLES20
import android.util.Log
import android.view.Surface


class VideoTexturePlayer(private val assetManager: AssetManager) {
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
    private var surface: Surface? = null
    private var mediaPlayer: MediaPlayer? = null

    public fun initialize(texName: Int, videoAssetPath: String) {
        cleanup()

        this.surfaceTexture = SurfaceTexture(texName)
        checkGLErrors("new SurfaceTexture")
        surface = Surface(surfaceTexture)
        checkGLErrors("new Surface")

        val mediaPlayer = MediaPlayer()
        this.mediaPlayer = mediaPlayer
        assetManager.openFd(videoAssetPath).use { videoDescriptor ->
            mediaPlayer.setDataSource(
                videoDescriptor.fileDescriptor,
                videoDescriptor.startOffset,
                videoDescriptor.length
            )
        }

        mediaPlayer.setSurface(surface)
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
    }

    private fun cleanup() {
        val mediaPlayer = this.mediaPlayer
        this.mediaPlayer = null
        if (mediaPlayer != null) {
            mediaPlayer.stop()
            mediaPlayer.release()
        }
        val surface = this.surface
        this.surface = null
        surface?.release()
        val surfaceTexture = this.surfaceTexture
        this.surfaceTexture = null
        surfaceTexture?.release()
        checkGLErrors("cleanup")
    }

    fun onPause() {
        mediaPlayer?.pause()
    }

    fun onResume() {
        mediaPlayer?.start()
    }

    fun onDestroy() {
        cleanup()
    }
}
