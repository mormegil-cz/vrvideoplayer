package cz.mormegil.vrvideoplayer

import android.content.Context
import android.graphics.SurfaceTexture
import android.graphics.SurfaceTexture.OnFrameAvailableListener
import android.media.AudioAttributes
import android.media.MediaPlayer
import android.media.MediaPlayer.OnVideoSizeChangedListener
import android.net.Uri
import android.os.Build
import android.util.Log
import android.view.Surface
import java.util.concurrent.atomic.AtomicBoolean

class VideoTexturePlayer(private val context: Context,
                         private val videoSourceUri: Uri,
    private val videoSizeChangedListener: OnVideoSizeChangedListener) :
    OnFrameAvailableListener {
    companion object {
        private const val TAG = "VRVideoPlayerV"
    }

    private var surfaceTexture: SurfaceTexture? = null
    private var mediaPlayer: MediaPlayer? = null
    private var videoPosition: Float = 0.0f
    private val frameAvailable: AtomicBoolean = AtomicBoolean(false)

    fun initializePlayback(texName: Int) {
        cleanup()

        val surfaceTexture = SurfaceTexture(texName)
        this.surfaceTexture = surfaceTexture
        surfaceTexture.setOnFrameAvailableListener(this);

        val mediaPlayer = MediaPlayer()
        this.mediaPlayer = mediaPlayer
        mediaPlayer.setOnVideoSizeChangedListener(videoSizeChangedListener);
        mediaPlayer.setDataSource(context, videoSourceUri)

        val surface = Surface(surfaceTexture)
        mediaPlayer.setSurface(surface)
        surface.release()

        mediaPlayer.prepare()
        /*
    mediaPlayer.setOnBufferingUpdateListener(this);
    mediaPlayer.setOnCompletionListener(this);
    mediaPlayer.setOnPreparedListener(this);
     */
        mediaPlayer.setAudioAttributes(
            AudioAttributes.Builder()
                .setContentType(AudioAttributes.CONTENT_TYPE_MOVIE)
                .build()
        )
        mediaPlayer.start()

        Log.d(TAG, "VideoTexturePlayer initialized with $videoSourceUri")
    }

    fun seek(relSeek: Int) {
        val mp = mediaPlayer ?: return
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            mp.seekTo(
                (mp.currentPosition + relSeek).toLong(),
                if (relSeek >= 0) MediaPlayer.SEEK_NEXT_SYNC else MediaPlayer.SEEK_PREVIOUS_SYNC
            );
        } else {
            mp.seekTo(mp.currentPosition + relSeek);
        }
    }

    fun getVideoPosition(): Float {
        return videoPosition
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
        // should always be non-null, but whatever
        val mp = mediaPlayer
        if (mp != null) {
            val position = mp.currentPosition.toFloat()
            val duration = mp.duration.coerceAtLeast(1).toFloat()
            videoPosition = position / duration;
        }

        frameAvailable.set(true)
    }

    fun updateIfNeeded() {
        if (frameAvailable.getAndSet(false)) {
            surfaceTexture?.updateTexImage()
        }
    }
}
