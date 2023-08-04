package cz.mormegil.vrvideoplayer

import android.os.Bundle
import android.util.Log
import android.view.Surface
import android.view.SurfaceHolder
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import cz.mormegil.vrvideoplayer.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity(), SurfaceHolder.Callback {
    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        Log.d(TAG, "onCreate()");

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val surfaceView = binding.surfaceview;
        surfaceView.holder.addCallback(this);
        surfaceView.setOnClickListener {
            val toast = Toast.makeText(
                this,
                "This demo combines Java UI and native EGL + OpenGL renderer",
                Toast.LENGTH_LONG
            )
            toast.show();
        };
    }

    override fun onStart() {
        super.onStart()
        Log.d(TAG, "onStart()")
        nativeOnStart()
    }

    override fun onResume() {
        super.onResume()
        Log.d(TAG, "onResume()")
        nativeOnResume()
    }

    override fun onPause() {
        super.onPause()
        Log.d(TAG, "onPause()")
        nativeOnPause()
    }

    override fun onStop() {
        super.onStop()
        Log.d(TAG, "onStop()")
        nativeOnStop()
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        nativeSetSurface(holder.surface)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, w: Int, h: Int) {
        nativeSetSurface(holder.surface)
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        nativeSetSurface(holder.surface)
    }

    private external fun nativeOnStart()
    private external fun nativeOnResume()
    private external fun nativeOnPause()
    private external fun nativeOnStop()
    private external fun nativeSetSurface(surface: Surface?)

    companion object {
        private const val TAG = "VrVideoPlayer"

        init {
            System.loadLibrary("vrvideoplayer")
        }
    }
}
