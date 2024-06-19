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
package com.fang.myapplication;

import android.content.Context;
import android.util.Log;
import android.view.Surface;

import com.fang.myapplication.model.NALPacket;
import com.fang.myapplication.model.PCMPacket;
import com.fang.myapplication.player.AVPlayer;

public class RaopServer {

    static {
        System.loadLibrary("raopserver_android");
    }

    private static final String TAG = "RaopServer";
    private long mServerId = 0;

    private long mBasePts = 0;
    private AVPlayer mAVPlayer;
    private Context mContext;
    private long byteLen = 0;
    private long frameCount = 0;
    private long lastTime = 0;

    public RaopServer(Context context) {
        mContext = context;
        mAVPlayer = new AVPlayer(context);
    }

    public void setSurface(Surface surface) {
        mAVPlayer.setSurface(surface);
    }

    public void onRecvVideoData(byte[] nal, int nalType, long dts, long pts, int width, int height) {
        Log.d(TAG, "onRecvVideoData pts = " + pts + ", nalType = " + nalType + ", width = " + width + ", height = " + height + ", nal length = " + nal.length);

        if (lastTime == 0) {
            lastTime = System.currentTimeMillis();
        }
        byteLen = byteLen + nal.length;
        frameCount++;
        long diffTime = System.currentTimeMillis() - lastTime;
        if (diffTime > 1000) {
            long fps = (long) (frameCount / ((float) diffTime / 1000));
            long speed = (long) (byteLen / 1024 / ((float) diffTime / 1000));
            Log.d("AVSPEED", "fps = " + fps + ", speed = " + speed);
            lastTime = System.currentTimeMillis();
            byteLen = 0;
            frameCount = 0;
        }

        if (mBasePts == 0) {
            mBasePts = pts;
        }
        NALPacket nalPacket = new NALPacket();
        nalPacket.nalData = nal;
        nalPacket.nalType = nalType;
        nalPacket.pts = pts - mBasePts;
        nalPacket.width = width;
        nalPacket.height = height;
        Log.d("AVSYNC", "recv video pts = " + nalPacket.pts);
        mAVPlayer.addPacker(nalPacket);
    }

    public void onRecvAudioData(short[] pcm, long pts) {
        Log.d(TAG, "onRecvAudioData pcm length = " + pcm.length + ", pts = " + pts);
        PCMPacket pcmPacket = new PCMPacket();
        pcmPacket.data = pcm;
        pcmPacket.pts = pts - mBasePts;
        Log.d("AVSYNC", "recv audio  pts = " + pcmPacket.pts);
        mAVPlayer.addPacker(pcmPacket);
    }

    /**
     * 视频销毁，代表一次连接结束，这个时候dnssd并没有销毁，可以继续连接
     */
    public void onRecvVideoDestroy() {
        Log.d(TAG, "onRecvideoDestroy");
        reset();
    }

    public void reset() {
        mBasePts = 0;
        mAVPlayer.reset();
    }

    public void startServer(String deviceName, byte[] hdAddr) {
        if (mServerId == 0) {
            mServerId = start(deviceName, hdAddr);
        }
        mAVPlayer.start();
    }

    public void stopServer() {
        if (mServerId != 0) {
            stop(mServerId);
        }
        mServerId = 0;
        mBasePts = 0;
        mAVPlayer.stop();
    }

    public int getPort() {
        if (mServerId != 0) {
            return getPort(mServerId);
        }
        return 0;
    }

    private native long start(String deviceName, byte[] hdAddr);
    private native void stop(long serverId);
    private native int getPort(long serverId);

}
