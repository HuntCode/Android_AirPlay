package com.fang.myapplication.player;

import android.media.AudioFormat;

import androidx.annotation.IntDef;

import java.lang.annotation.Documented;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public final class Constant {
    /**
     * Special constant representing a time corresponding to the end of a source. Suitable for use in
     * any time base.
     */
    public static final long TIME_END_OF_SOURCE = Long.MIN_VALUE;

    /**
     * Special constant representing an unset or unknown time or duration. Suitable for use in any
     * time base.
     */
    public static final long TIME_UNSET = Long.MIN_VALUE + 1;

    /**
     * Represents an unset or unknown index.
     */
    public static final int INDEX_UNSET = -1;

    /**
     * Represents an unset or unknown position.
     */
    public static final int POSITION_UNSET = -1;

    /**
     * Represents an unset or unknown length.
     */
    public static final int LENGTH_UNSET = -1;

    /** Represents an unset or unknown percentage. */
    public static final int PERCENTAGE_UNSET = -1;

    /**
     * The number of microseconds in one second.
     */
    public static final long MICROS_PER_SECOND = 1000000L;

    /**
     * The number of nanoseconds in one second.
     */
    public static final long NANOS_PER_SECOND = 1000000000L;

    /** The number of bits per byte. */
    public static final int BITS_PER_BYTE = 8;

    /** The number of bytes per float. */
    public static final int BYTES_PER_FLOAT = 4;

    /**
     * The name of the ASCII charset.
     */
    public static final String ASCII_NAME = "US-ASCII";
    /**
     * The name of the UTF-8 charset.
     */
    public static final String UTF8_NAME = "UTF-8";

    /**
     * The name of the UTF-16 charset.
     */
    public static final String UTF16_NAME = "UTF-16";

    /** The name of the UTF-16 little-endian charset. */
    public static final String UTF16LE_NAME = "UTF-16LE";

    /**
     * The name of the serif font family.
     */
    public static final String SERIF_NAME = "serif";

    /**
     * The name of the sans-serif font family.
     */
    public static final String SANS_SERIF_NAME = "sans-serif";
    /**
     * Represents an audio encoding, or an invalid or unset value. One of {@link Format#NO_VALUE},
     * {@link #ENCODING_INVALID}, {@link #ENCODING_PCM_8BIT}, {@link #ENCODING_PCM_16BIT}, {@link
     * #ENCODING_PCM_24BIT}, {@link #ENCODING_PCM_32BIT}, {@link #ENCODING_PCM_FLOAT}, {@link
     * #ENCODING_PCM_MU_LAW}, {@link #ENCODING_PCM_A_LAW}, {@link #ENCODING_AC3}, {@link
     * #ENCODING_E_AC3}, {@link #ENCODING_E_AC3_JOC}, {@link #ENCODING_AC4}, {@link #ENCODING_DTS},
     * {@link #ENCODING_DTS_HD} or {@link #ENCODING_DOLBY_TRUEHD}.
     */
    @Documented
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
            ENCODING_INVALID,
            ENCODING_PCM_8BIT,
            ENCODING_PCM_16BIT,
            ENCODING_PCM_24BIT,
            ENCODING_PCM_32BIT,
            ENCODING_PCM_FLOAT,
            ENCODING_PCM_MU_LAW,
            ENCODING_PCM_A_LAW,
            ENCODING_AC3,
            ENCODING_E_AC3,
            ENCODING_E_AC3_JOC,
            ENCODING_AC4,
            ENCODING_DTS,
            ENCODING_DTS_HD,
            ENCODING_DOLBY_TRUEHD,
    })
    public @interface Encoding {}

    /**
     * Represents a PCM audio encoding, or an invalid or unset value. One of {@link Format#NO_VALUE},
     * {@link #ENCODING_INVALID}, {@link #ENCODING_PCM_8BIT}, {@link #ENCODING_PCM_16BIT}, {@link
     * #ENCODING_PCM_24BIT}, {@link #ENCODING_PCM_32BIT}, {@link #ENCODING_PCM_FLOAT}, {@link
     * #ENCODING_PCM_MU_LAW} or {@link #ENCODING_PCM_A_LAW}.
     */
    @Documented
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
            ENCODING_INVALID,
            ENCODING_PCM_8BIT,
            ENCODING_PCM_16BIT,
            ENCODING_PCM_24BIT,
            ENCODING_PCM_32BIT,
            ENCODING_PCM_FLOAT,
            ENCODING_PCM_MU_LAW,
            ENCODING_PCM_A_LAW
    })
    public @interface PcmEncoding {}
    /** @see AudioFormat#ENCODING_INVALID */
    public static final int ENCODING_INVALID = AudioFormat.ENCODING_INVALID;
    /** @see AudioFormat#ENCODING_PCM_8BIT */
    public static final int ENCODING_PCM_8BIT = AudioFormat.ENCODING_PCM_8BIT;
    /** @see AudioFormat#ENCODING_PCM_16BIT */
    public static final int ENCODING_PCM_16BIT = AudioFormat.ENCODING_PCM_16BIT;
    /** PCM encoding with 24 bits per sample. */
    public static final int ENCODING_PCM_24BIT = 0x80000000;
    /** PCM encoding with 32 bits per sample. */
    public static final int ENCODING_PCM_32BIT = 0x40000000;
    /** @see AudioFormat#ENCODING_PCM_FLOAT */
    public static final int ENCODING_PCM_FLOAT = AudioFormat.ENCODING_PCM_FLOAT;
    /** Audio encoding for mu-law. */
    public static final int ENCODING_PCM_MU_LAW = 0x10000000;
    /** Audio encoding for A-law. */
    public static final int ENCODING_PCM_A_LAW = 0x20000000;
    /** @see AudioFormat#ENCODING_AC3 */
    public static final int ENCODING_AC3 = AudioFormat.ENCODING_AC3;
    /** @see AudioFormat#ENCODING_E_AC3 */
    public static final int ENCODING_E_AC3 = AudioFormat.ENCODING_E_AC3;
    /** @see AudioFormat#ENCODING_E_AC3_JOC */
    public static final int ENCODING_E_AC3_JOC = AudioFormat.ENCODING_E_AC3_JOC;
    /** @see AudioFormat#ENCODING_AC4 */
    public static final int ENCODING_AC4 = AudioFormat.ENCODING_AC4;
    /** @see AudioFormat#ENCODING_DTS */
    public static final int ENCODING_DTS = AudioFormat.ENCODING_DTS;
    /** @see AudioFormat#ENCODING_DTS_HD */
    public static final int ENCODING_DTS_HD = AudioFormat.ENCODING_DTS_HD;
    /** @see AudioFormat#ENCODING_DOLBY_TRUEHD */
    public static final int ENCODING_DOLBY_TRUEHD = AudioFormat.ENCODING_DOLBY_TRUEHD;


    /**
     * Converts a time in microseconds to the corresponding time in milliseconds, preserving
     * {@link #TIME_UNSET} and {@link #TIME_END_OF_SOURCE} values.
     *
     * @param timeUs The time in microseconds.
     * @return The corresponding time in milliseconds.
     */
    public static long usToMs(long timeUs) {
        return (timeUs == TIME_UNSET || timeUs == TIME_END_OF_SOURCE) ? timeUs : (timeUs / 1000);
    }
}
