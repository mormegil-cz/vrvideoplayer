package cz.mormegil.vrvideoplayer

import android.media.AudioManager

class Controller(
    private val audioManager: AudioManager,
    private val videoTexturePlayer: VideoTexturePlayer
) {
    private val buttonRecenter2D: Int = 1
    private val buttonRecenterYaw: Int = 2
    private val buttonVolumeDown: Int = 3
    private val buttonVolumeUp: Int = 4
    private val buttonOpenFile: Int = 5
    private val buttonPlay: Int = 6
    private val buttonBack: Int = 7
    private val buttonForward: Int = 8
    private val buttonRewind: Int = 9
    private val buttonPause: Int = 10

    fun executeButtonAction(action: Int) {
        when (action) {
            buttonVolumeDown -> {
                audioManager.adjustStreamVolume(
                    AudioManager.STREAM_MUSIC,
                    AudioManager.ADJUST_LOWER,
                    0
                )
            }

            buttonVolumeUp -> {
                audioManager.adjustStreamVolume(
                    AudioManager.STREAM_MUSIC,
                    AudioManager.ADJUST_RAISE,
                    0
                )
            }

            buttonForward -> {
                videoTexturePlayer.seek(5000)
            }

            buttonBack -> {
                videoTexturePlayer.seek(-5000)
            }

            buttonRewind -> {
                videoTexturePlayer.rewind()
            }
        }
    }
}