package com.maxi.gbaemu

import android.graphics.Bitmap

object EmulatorBridge {
    init {
        System.loadLibrary("native-lib")
    }

    external fun loadRom(romPath: String): Boolean
    external fun runFrame()
    external fun getVideoBuffer(bitmap: Bitmap): Bitmap
    external fun setKeyState(key: Int, pressed: Boolean)
    external fun getVideoWidth(): Int
    external fun getVideoHeight(): Int

    object Keys {
        const val A = 0
        const val B = 1
        const val SELECT = 2
        const val START = 3
        const val RIGHT = 4
        const val LEFT = 5
        const val UP = 6
        const val DOWN = 7
        const val R = 8
        const val L = 9
    }
}