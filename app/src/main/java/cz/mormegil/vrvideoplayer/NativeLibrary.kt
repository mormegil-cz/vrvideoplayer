package cz.mormegil.vrvideoplayer

import android.content.Context
import android.content.res.AssetManager

object NativeLibrary {
    external fun nativeInit(
        context: Context,
        assets: AssetManager,
        videoTexturePlayer: VideoTexturePlayer
    ): Long

    external fun nativeOnResume(nativeApp: Long)
    external fun nativeOnPause(nativeApp: Long)
    external fun nativeOnDestroy(nativeApp: Long)
    external fun nativeOnSurfaceCreated(nativeApp: Long)
    external fun nativeSetScreenParams(nativeApp: Long, width: Int, height: Int)
    external fun nativeScanCardboardQr(nativeApp: Long);
    external fun nativeDrawFrame(nativeApp: Long);

    init {
        System.loadLibrary("vrvideoplayer")
    }
}
