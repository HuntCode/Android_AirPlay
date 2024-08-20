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
import android.view.Surface;

import com.fang.myapplication.MainActivity;
import com.fang.myapplication.model.NALPacket;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class VideoPlayer {

    private static final String TAG = "VideoPlayer";

    private static final int VIDEO_WIDTH = 540;
    private static final int VIDEO_HEIGHT = 960;
    private String mMimeType = "video/avc";
    private MediaCodec mCodec = null;
    private List<NALPacket> mSyncList = Collections.synchronizedList(new ArrayList<>());

    private MediaTimeProvider mMediaTimeProvider;
    private MediaCodecVideoRenderer mRender;
    private Context mContext;
    private Surface mSurface;

    public VideoPlayer(Context context, MediaTimeProvider mediaTimeProvider) {
        mContext = context;
        mMediaTimeProvider = mediaTimeProvider;
    }

    public void setSurface(Surface surface) {
        this.mSurface = surface;
    }

    public void start() {
        try {
            if (mCodec != null) {
                mCodec.stop();
                mCodec.release();
            }
            MediaFormat format = MediaFormat.createVideoFormat(mMimeType, VIDEO_WIDTH, VIDEO_HEIGHT);
            mCodec = MediaCodec.createDecoderByType(mMimeType);
            mCodec.configure(format, mSurface, null, 0);
            mCodec.setVideoScalingMode(MediaCodec.VIDEO_SCALING_MODE_SCALE_TO_FIT);
            mCodec.start();
            mRender = new MediaCodecVideoRenderer(mContext, mCodec, this);
            mRender.start();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void reset() {
        mSyncList.clear();
        if (mRender != null) {
            mRender.reset();
        }
    }

    public void stop() {
        reset();
        if (mRender != null) {
            mRender.stop();
        }
        if (mCodec != null) {
            mCodec.stop();
            mCodec.release();
            mCodec = null;
        }
    }

    public void addPacker(NALPacket nalPacket) {
        mSyncList.add(nalPacket);
    }

    // 读取数据
    public NALPacket getPacket() {
        if (!mSyncList.isEmpty()) {
            return mSyncList.remove(0);
        }
        return null;
    }

    public long getAudioUs() {
        return mMediaTimeProvider.getAudioUs();
    }

    public void setStartTime(long time) {
        mMediaTimeProvider.setStartTime(time);
    }

    void doSomeWork() {
        mRender.render(getAudioUs(), SystemClock.elapsedRealtime() * 1000);
    }

    public void processOutputFormat(int width, int height) {
        Log.d("AVSPEED", "width = " + width + ", height = " + height);
        ((MainActivity) mContext).processOutputFormat(width, height);
    }
}
