package cz.mormegil.vrvideoplayer

import android.content.Context
import android.content.res.AssetManager

object NativeLibrary {
    external fun nativeInit(
        context: Context,
        assetManager: AssetManager,
        videoTexturePlayer: VideoTexturePlayer,
        controller: Controller
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
    None {
        override fun menuItemId(): Int = throw IllegalArgumentException()
    },
    Mono {
        override fun menuItemId(): Int = R.id.input_layout_mono
    },
    StereoHoriz {
        override fun menuItemId(): Int = R.id.input_layout_stereo_horiz
    },
    StereoVert {
        override fun menuItemId(): Int = R.id.input_layout_stereo_vert
    },
    AnaglyphRedCyan {
        override fun menuItemId(): Int = R.id.input_layout_anaglyph_red_cyan
    };

    abstract fun menuItemId(): Int
}

enum class InputMode {
    None {
        override fun menuItemId(): Int = throw IllegalArgumentException()
    },
    PlainFov {
        override fun menuItemId(): Int = R.id.input_mode_plain_fov
    },
    Equirect180 {
        override fun menuItemId(): Int = R.id.input_mode_equirect_180
    },
    Equirect360 {
        override fun menuItemId(): Int = R.id.input_mode_equirect_360
    },
    CubeMap {
        override fun menuItemId(): Int = throw IllegalArgumentException()
    },
    EquiangCubeMap {
        override fun menuItemId(): Int = throw IllegalArgumentException()
    },
    Pyramid {
        override fun menuItemId(): Int = throw IllegalArgumentException()
    },
    Panorama180 {
        override fun menuItemId(): Int = R.id.input_mode_panorama_180
    },
    Panorama360 {
        override fun menuItemId(): Int = R.id.input_mode_panorama_360
    };

    abstract fun menuItemId(): Int
}

enum class OutputMode {
    None {
        override fun menuItemId(): Int = throw IllegalArgumentException()
    },
    MonoLeft {
        override fun menuItemId(): Int = R.id.output_mode_mono_left_eye
    },
    MonoRight {
        override fun menuItemId(): Int = R.id.output_mode_mono_right_eye
    },
    CardboardStereo {
        override fun menuItemId(): Int = R.id.output_mode_cardboard
    };

    abstract fun menuItemId(): Int
}
