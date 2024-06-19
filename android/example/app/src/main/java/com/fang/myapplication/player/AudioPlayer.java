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

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.SystemClock;
import android.util.Log;

import com.fang.myapplication.model.PCMPacket;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class AudioPlayer {

    private static final String TAG = "AudioPlayer";

    private AudioTrack mAudioTrack;
    private List<PCMPacket> mSyncList = Collections.synchronizedList(new ArrayList<>());
    private volatile boolean mIsStart = false;
    private final AudioTrackPositionTracker audioTrackPositionTracker;
    private long firstPts = 0;
    private long initUs = 0;
    int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
    int sampleRate = 44100;
    int channel = AudioFormat.CHANNEL_OUT_STEREO;
    public AudioPlayer() {
        mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate, channel, audioFormat,
                AudioTrack.getMinBufferSize(sampleRate, channel, audioFormat), AudioTrack.MODE_STREAM);

        audioTrackPositionTracker = new AudioTrackPositionTracker(new com.fang.myapplication.player.AudioTrackPositionTracker.Listener() {
            @Override
            public void onPositionFramesMismatch(long audioTimestampPositionFrames, long audioTimestampSystemTimeUs, long systemTimeUs, long playbackPositionUs) {

            }

            @Override
            public void onSystemTimeUsMismatch(long audioTimestampPositionFrames, long audioTimestampSystemTimeUs, long systemTimeUs, long playbackPositionUs) {

            }

            @Override
            public void onInvalidLatency(long latencyUs) {

            }

            @Override
            public void onUnderrun(int bufferSize, long bufferSizeMs) {

            }
        });
        audioTrackPositionTracker.setAudioTrack(mAudioTrack, Constant.ENCODING_PCM_16BIT, 4, AudioTrack.getMinBufferSize(sampleRate, channel, audioFormat));
    }

    public void addPacker(PCMPacket pcmPacket) {
        mSyncList.add(pcmPacket);
        if (firstPts == 0) {
            firstPts = pcmPacket.pts;
        }
    }

    public void doSomeWork() {
        if (!mSyncList.isEmpty()) {
            PCMPacket pcmPacket = mSyncList.remove(0);
            Log.d("AVSYNC","audio process pts = " + pcmPacket.pts);
            mAudioTrack.write(pcmPacket.data, 0, 960);
        }
    }

    private byte[] short2byte(short[] sData) {
        int shortArrsize = sData.length;
        byte[] bytes = new byte[shortArrsize * 2];
        for (int i = 0; i < shortArrsize; i++) {
            bytes[i * 2] = (byte) (sData[i] & 0x00FF);
            bytes[(i * 2) + 1] = (byte) (sData[i] >> 8);
            sData[i] = 0;
        }
        return bytes;
    }

    public void start() {
        mIsStart = true;
        if (mAudioTrack == null) {
            mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate, channel, audioFormat,
                    AudioTrack.getMinBufferSize(sampleRate, channel, audioFormat), AudioTrack.MODE_STREAM);
        }
        audioTrackPositionTracker.start();
        mAudioTrack.play();
    }

    public void reset() {
        firstPts = 0;
        initUs = 0;
        mSyncList.clear();
    }

    public void stop() {
        reset();
        if (mAudioTrack != null) {
            mAudioTrack.flush();
            mAudioTrack.stop();
            mAudioTrack.release();
            mAudioTrack = null;
        }
        mIsStart = false;
    }

    public long getPositionUs() {
        if (mIsStart && firstPts != 0) {
            Log.d("AVSYNC","use audio pts");
            return initUs + firstPts + audioTrackPositionTracker.getCurrentPositionUs(false);
        }
        return SystemClock.elapsedRealtime() * 1000;
    }

    public void setStartTime(long time) {
        initUs = time;
    }

    public boolean isStart() {
        return mIsStart;
    }
}
