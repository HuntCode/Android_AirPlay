package com.fang.myapplication.util;

import android.os.Build;

import androidx.annotation.Nullable;

import com.fang.myapplication.player.Constant;

public final class Util {
    /**
     * Like {@link android.os.Build.VERSION#SDK_INT}, but in a place where it can be conveniently
     * overridden for local testing.
     */
    public static final int SDK_INT = Build.VERSION.SDK_INT;
    
    /**
     * Returns whether {@code encoding} is one of the linear PCM encodings.
     *
     * @param encoding The encoding of the audio data.
     * @return Whether the encoding is one of the PCM encodings.
     */
    public static boolean isEncodingLinearPcm(@Constant.Encoding int encoding) {
        return encoding == Constant.ENCODING_PCM_8BIT
                || encoding == Constant.ENCODING_PCM_16BIT
                || encoding == Constant.ENCODING_PCM_24BIT
                || encoding == Constant.ENCODING_PCM_32BIT
                || encoding == Constant.ENCODING_PCM_FLOAT;
    }

    public static <T> T castNonNull(@Nullable T value) {
        return value;
    }
}
