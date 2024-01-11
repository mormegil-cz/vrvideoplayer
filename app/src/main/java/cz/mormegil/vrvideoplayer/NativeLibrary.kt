package cz.mormegil.vrvideoplayer

import android.content.Context
import android.content.res.AssetManager

object NativeLibrary {
    external fun nativeInit(
        context: Context,
        assetManager: AssetManager,
        videoTexturePlayer: VideoTexturePlayer
    ): Long

    external fun nativeOnResume(nativeApp: Long)
    external fun nativeOnPause(nativeApp: Long)
    external fun nativeOnDestroy(nativeApp: Long)
    external fun nativeOnSurfaceCreated(nativeApp: Long)
    external fun nativeSetScreenParams(nativeApp: Long, width: Int, height: Int)
    external fun nativeOnVideoSizeChanged(nativeApp: Long, width: Int, height: Int)
    external fun nativeScanCardboardQr(nativeApp: Long)
    external fun nativeShowProgressBar(nativeApp: Long)
    external fun nativeSetOptions(
        nativeApp: Long,
        inputLayout: Int,
        inputMode: Int,
        outputMode: Int
    )
    external fun nativeDrawFrame(
        nativeApp: Long,
        videoPosition: Float
    )

    init {
        System.loadLibrary("vrvideoplayer")
    }
}

enum class InputLayout {
    None,
    Mono,
    StereoHoriz,
    StereoVert,
    AnaglyphRedCyan,
}

enum class InputMode {
    None,
    PlainFov,
    Equirect180,
    Equirect360,
    CubeMap,
    EquiangCubeMap,
    Pyramid,
    Panorama180,
    Panorama360,
}

enum class OutputMode {
    None,
    MonoLeft,
    MonoRight,
    CardboardStereo,
}
