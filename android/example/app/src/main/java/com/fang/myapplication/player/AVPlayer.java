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
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.view.Surface;

import com.fang.myapplication.model.NALPacket;
import com.fang.myapplication.model.PCMPacket;

public class AVPlayer implements MediaTimeProvider, Handler.Callback {

    private static final String TAG = "AVPlayer";

    private static final int MSG_DO_SOME_WORK = 2;

    private VideoPlayer mVideoPlayer;
    private AudioPlayer mAudioPlayer;
    private Handler mHandler;
    private HandlerThread mRenderThread;

    public AVPlayer(Context context) {
        mAudioPlayer = new AudioPlayer();
        mVideoPlayer = new VideoPlayer(context, this);
        mRenderThread = new HandlerThread("Render");
        mRenderThread.start();
        mHandler = new Handler(mRenderThread.getLooper(), this);
    }

    public void start() {
        mVideoPlayer.start();
        mAudioPlayer.start();
        mHandler.sendEmptyMessage(MSG_DO_SOME_WORK);
    }

    public void reset() {
        mVideoPlayer.reset();
        mAudioPlayer.reset();
    }

    public void stop() {
        mHandler.removeCallbacks(null);
        mVideoPlayer.stop();
        mAudioPlayer.stop();
    }

    public void setSurface(Surface surface) {
        mVideoPlayer.setSurface(surface);
    }

    @Override
    public boolean handleMessage(Message msg) {
        switch (msg.what) {
            case MSG_DO_SOME_WORK:
                doSomeWork();
                break;
        }
        return false;
    }

    /* video */
    public void addPacker(NALPacket nalPacket) {
        mVideoPlayer.addPacker(nalPacket);
    }

    /* audio */
    public void addPacker(PCMPacket pcmPacket) {
        mAudioPlayer.addPacker(pcmPacket);
    }

    private void doSomeWork() {
        mVideoPlayer.doSomeWork();
        mAudioPlayer.doSomeWork();
        mHandler.sendEmptyMessageDelayed(MSG_DO_SOME_WORK, 5);
    }

    @Override
    public long getAudioUs() {
        return mAudioPlayer.getPositionUs();
    }

    @Override
    public void setStartTime(long time) {
        mAudioPlayer.setStartTime(time);
    }

    public void destroy() {
        stop();
        mRenderThread.quit();
    }

}
