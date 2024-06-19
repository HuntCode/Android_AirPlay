/*
 * MIT License
 *
 * Copyright (c) 2019 dsafa22, All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
package com.fang.myapplication.player;

import android.content.Context;
import android.media.MediaCodec;
import android.media.MediaFormat;
import android.os.SystemClock;
import android.util.Log;

import com.fang.myapplication.model.NALPacket;

import java.nio.ByteBuffer;

public class MediaCodecVideoRenderer {

    private int inputIndex;
    private int outputIndex = -1;
    private MediaCodec codec;
    private ByteBuffer outputBuffer;
    private MediaCodec.BufferInfo outputBufferInfo = new MediaCodec.BufferInfo();

    private static final String KEY_CROP_LEFT = "crop-left";
    private static final String KEY_CROP_RIGHT = "crop-right";
    private static final String KEY_CROP_BOTTOM = "crop-bottom";
    private static final String KEY_CROP_TOP = "crop-top";

    private VideoFrameReleaseTimeHelper frameReleaseTimeHelper;
    private boolean renderedFirstFrame;
    private long lastRenderTimeUs;

    private VideoPlayer videoPlayer;
    private long initPositionUs = 0;
    private volatile boolean mIsStart = false;

    public MediaCodecVideoRenderer(Context context, MediaCodec codec, VideoPlayer videoPlayer) {
        this.codec = codec;
        frameReleaseTimeHelper = new VideoFrameReleaseTimeHelper(context.getApplicationContext());
        this.videoPlayer = videoPlayer;
        resetInputBuffer();
        resetOutputBuffer();
    }

    /**
     * 开始渲染
     * @param positionUs 同步时间戳
     * @param elapsedRealtimeUs 当前时间
     */
    public void render(long positionUs, long elapsedRealtimeUs) {
        while (drainOutputBuffer(positionUs, elapsedRealtimeUs)) {}
        while (feedInputBuffer(positionUs)) {}
    }

    public boolean feedInputBuffer(long positionUs) {
        if (codec == null || !mIsStart) {
            return false;
        }
        if (inputIndex < 0) {
            inputIndex = codec.dequeueInputBuffer(0);
            if (inputIndex < 0) {
                return false;
            }
        }
        NALPacket nalPacket = videoPlayer.getPacket();
        if (nalPacket == null) {
            return false;
        }
        if (initPositionUs == 0) {
            initPositionUs = positionUs;
            videoPlayer.setStartTime(positionUs);
        }
        ByteBuffer inputBuffer = codec.getInputBuffer(inputIndex);
        inputBuffer.put(nalPacket.nalData);
        long pts = nalPacket.pts + initPositionUs;
        Log.d("AVSYNC_VIDEO", "input video pts = " + pts + ", initPositionUs = " + initPositionUs);
        codec.queueInputBuffer(inputIndex, 0, nalPacket.nalData.length, pts, 0);
        resetInputBuffer();
        return true;
    }

    public void start() {
        mIsStart = true;
    }

    public void reset() {
        initPositionUs = 0;
        renderedFirstFrame = false;
        resetInputBuffer();
        resetOutputBuffer();
    }

    public void stop() {
        reset();
        mIsStart = false;
    }

    private boolean drainOutputBuffer(long positionUs, long elapsedRealtimeUs) {
        if (!mIsStart) {
            return false;
        }
        if (!hasOutputBuffer()) {
            int outputIndex = codec.dequeueOutputBuffer(outputBufferInfo, 0);
            if (outputIndex < 0) {
                if (outputIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED /* (-2) */) {
                    processOutputFormat();
                    return true;
                } else if (outputIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED /* (-3) */) {
                    processOutputBuffersChanged();
                    return true;
                }
                return false;
            }
            this.outputIndex = outputIndex;
            outputBuffer = codec.getOutputBuffer(outputIndex);
            if (outputBuffer != null) {
                outputBuffer.position(outputBufferInfo.offset);
                outputBuffer.limit(outputBufferInfo.offset + outputBufferInfo.size);
            }
        }
        boolean processedOutputBuffer = processOutputBuffer(positionUs, elapsedRealtimeUs, outputIndex, outputBufferInfo.presentationTimeUs);
        if (processedOutputBuffer) {
            resetOutputBuffer();
            return true;
        }
        return false;
    }

    private boolean processOutputBuffer(long positionUs, long elapsedRealtimeUs, int bufferIndex, long bufferPresentationTimeUs) {
        if (codec == null) {
            return false;
        }
        // 展示时间
        long presentationTimeUs = bufferPresentationTimeUs;
        //Log.d("AVSYNC_VIDEO", "output video pts = " + presentationTimeUs + ", positionUs = " + positionUs);
        long earlyUs = bufferPresentationTimeUs - positionUs;
        Log.d("AVSYNC_VIDEO", "earlyUs 1 = " + earlyUs);
        long elapsedRealtimeNowUs = SystemClock.elapsedRealtime() * 1000;
        if (!renderedFirstFrame
                || shouldForceRenderOutputBuffer(earlyUs, elapsedRealtimeNowUs - lastRenderTimeUs)) {
            long releaseTimeNs = System.nanoTime();
            Log.d("AVSYNC_VIDEO", "earlyUs show 1 = " + earlyUs);
            renderOutputBufferV21(codec, bufferIndex, presentationTimeUs, releaseTimeNs);
            return true;
        }

        long elapsedSinceStartOfLoopUs = elapsedRealtimeNowUs - elapsedRealtimeUs;
        earlyUs -= elapsedSinceStartOfLoopUs;
        Log.d("AVSYNC_VIDEO", "earlyUs 2 = " + earlyUs);

        // Compute the buffer's desired release time in nanoseconds.
//        long systemTimeNs = System.nanoTime();
//        long unadjustedFrameReleaseTimeNs = systemTimeNs + (earlyUs * 1000);
//
//        // Apply a timestamp adjustment, if there is one.
//        long adjustedReleaseTimeNs = frameReleaseTimeHelper.adjustReleaseTime(
//                bufferPresentationTimeUs, unadjustedFrameReleaseTimeNs);
//        earlyUs = (adjustedReleaseTimeNs - systemTimeNs) / 1000;


        //Log.d("AVSYNC_VIDEO", "earlyUs 3 = " + earlyUs);
        if (shouldDropBuffersToKeyframe(earlyUs, elapsedRealtimeUs)
                && maybeDropBuffersToKeyframe(codec, bufferIndex, presentationTimeUs, positionUs)) {
            Log.d("AVSYNC_VIDEO", "earlyUs drop 1 = " + earlyUs);
            return false;
        } else if (shouldDropOutputBuffer(earlyUs, elapsedRealtimeUs)) {
            dropOutputBuffer(codec, bufferIndex, presentationTimeUs);
            Log.d("AVSYNC_VIDEO", "earlyUs drop 2 = " + earlyUs);
            return true;
        }

        // Let the underlying framework time the release.
        if (earlyUs < 400_000) {
            Log.d("AVSYNC_VIDEO", "earlyUs show 2 = " + earlyUs);
            renderOutputBufferV21(codec, bufferIndex, presentationTimeUs, 0);
            return true;
        }
        // We're either not playing, or it's not time to render the frame yet.
        return false;
    }

    public void simpleRender(NALPacket nalPacket) {
        int inputIndex = codec.dequeueInputBuffer(0);
        if (inputIndex < 0) {
            return;
        }
        ByteBuffer inputBuffer = codec.getInputBuffer(inputIndex);
        inputBuffer.put(nalPacket.nalData);
        codec.queueInputBuffer(inputIndex, 0, nalPacket.nalData.length, nalPacket.pts, 0);
        int outputIndex = codec.dequeueOutputBuffer(outputBufferInfo, 0);
        if (outputIndex < 0) {
            if (outputIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED /* (-2) */) {
                processOutputFormat();
                return;
            } else if (outputIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED /* (-3) */) {
                processOutputBuffersChanged();
                return;
            }
            return;
        }
        codec.releaseOutputBuffer(outputIndex, true);
    }

    private void resetInputBuffer() {
        inputIndex = -1;
    }

    private void resetOutputBuffer() {
        outputIndex = -1;
        outputBuffer = null;
    }

    private boolean hasOutputBuffer() {
        return outputIndex >= 0;
    }

    /**
     * Processes a new output format.
     */
    private void processOutputFormat() {
        MediaFormat outputFormat = codec.getOutputFormat();
        boolean hasCrop = outputFormat.containsKey(KEY_CROP_RIGHT)
                && outputFormat.containsKey(KEY_CROP_LEFT) && outputFormat.containsKey(KEY_CROP_BOTTOM)
                && outputFormat.containsKey(KEY_CROP_TOP);
        int width = hasCrop ? outputFormat.getInteger(KEY_CROP_RIGHT) - outputFormat.getInteger(KEY_CROP_LEFT) + 1
                        : outputFormat.getInteger(MediaFormat.KEY_WIDTH);
        int height = hasCrop ? outputFormat.getInteger(KEY_CROP_BOTTOM) - outputFormat.getInteger(KEY_CROP_TOP) + 1
                        : outputFormat.getInteger(MediaFormat.KEY_HEIGHT);
        videoPlayer.processOutputFormat(width, height);
    }

    private void processOutputBuffersChanged() {
//        if (Util.SDK_INT < 21) {
//            outputBuffers = codec.getOutputBuffers();
//        }
    }

    // 丢掉整个gop
    private boolean maybeDropBuffersToKeyframe(MediaCodec codec, int index, long presentationTimeUs,
                                                 long positionUs) {

        return false;
    }

    private boolean shouldForceRenderOutputBuffer(long earlyUs, long elapsedSinceLastRenderUs) {
        return isBufferLate(earlyUs) && elapsedSinceLastRenderUs > 100000;
    }

    private boolean shouldDropBuffersToKeyframe(long earlyUs, long elapsedRealtimeUs) {
        return isBufferVeryLate(earlyUs);
    }

    private static boolean isBufferVeryLate(long earlyUs) {
        // Class a buffer as very late if it should have been presented more than 500 ms ago.
        return earlyUs < -500000;
    }

    private boolean shouldDropOutputBuffer(long earlyUs, long elapsedRealtimeUs) {
        return isBufferLate(earlyUs);
    }

    private static boolean isBufferLate(long earlyUs) {
        // Class a buffer as late if it should have been presented more than 300 ms ago.
        return earlyUs < -400000;
    }

    /**
     * Drops the output buffer with the specified index.
     *
     * @param codec The codec that owns the output buffer.
     * @param index The index of the output buffer to drop.
     * @param presentationTimeUs The presentation time of the output buffer, in microseconds.
     */
    protected void dropOutputBuffer(MediaCodec codec, int index, long presentationTimeUs) {
        codec.releaseOutputBuffer(index, false);
    }

    protected void renderOutputBufferV21(MediaCodec codec, int index, long presentationTimeUs, long releaseTimeNs) {
        //codec.releaseOutputBuffer(index, releaseTimeNs);
        codec.releaseOutputBuffer(index, true);
        lastRenderTimeUs = SystemClock.elapsedRealtime() * 1000;
        if (!renderedFirstFrame) {
            renderedFirstFrame = true;
        }
    }

}
