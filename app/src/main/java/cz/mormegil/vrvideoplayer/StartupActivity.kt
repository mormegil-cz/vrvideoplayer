package cz.mormegil.vrvideoplayer

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.result.PickVisualMediaRequest
import androidx.activity.result.contract.ActivityResultContracts

class StartupActivity : ComponentActivity() {
    companion object {
        private const val TAG = "VRVideoPlayerS"
    }

    private val videoGalleryChooser =
        registerForActivityResult(ActivityResultContracts.PickVisualMedia(), ::initWithVideo)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        Log.d(TAG, "onCreate()")

        when (intent.action) {
            Intent.ACTION_VIEW -> {
                val viewUri = intent.data
                if (viewUri != null) {
                    initWithVideo(viewUri)
                } else {
                    chooseVideoFromGallery()
                }
            }

            // Intent.ACTION_MAIN ->
            else -> {
                chooseVideoFromGallery()
            }
        }
    }

    private fun chooseVideoFromGallery() {
        videoGalleryChooser.launch(PickVisualMediaRequest(ActivityResultContracts.PickVisualMedia.VideoOnly))
    }

    private fun initWithVideo(videoUri: Uri?) {
        if (videoUri == null) {
            // bah!
            Log.d(TAG, "No video chosen, aborting")
            finish()
        }
        Log.d(TAG, "initWithVideo: $videoUri")

        val intent = Intent(this, MainActivity::class.java)
        intent.data = videoUri
        startActivity(intent)
        finish()
    }
}
