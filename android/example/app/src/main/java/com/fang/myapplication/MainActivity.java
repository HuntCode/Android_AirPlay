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

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.graphics.SurfaceTexture;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity implements View.OnClickListener, TextureView.SurfaceTextureListener {

    public static String TAG = "MainActivity";

    private static final int MSG_VIDEO_SIZE = 1;
    private RaopServer mRaopServer;
    private TextureView mTextureView;
    private Button mBtnControl;
    private TextView mTxtDevice;
    private boolean mIsStart = false;
    private byte[] mMacAddress;

    private String mDeviceName;
    private int mDeviceTail = 1;

    private int mScreenWidth;
    private int mScreenHeight;
    private Handler mHandler = new Handler(new Handler.Callback() {
        @Override
        public boolean handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_VIDEO_SIZE:
                    updateTextureView(msg.arg1, msg.arg2);
                    break;
            }
            return true;
        }
    });

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        getSystemService(Context.NSD_SERVICE);
        mBtnControl = findViewById(R.id.btn_control);
        mTxtDevice = findViewById(R.id.txt_device);
        mBtnControl.setOnClickListener(this);
        mTextureView = findViewById(R.id.surface);
        mRaopServer = new RaopServer(MainActivity.this);
        mMacAddress = NetUtils.getLocalMacAddress();
        mTextureView.setSurfaceTextureListener(this);
        DisplayMetrics displayMetrics = getResources().getDisplayMetrics();
        mScreenWidth = displayMetrics.widthPixels;
        mScreenHeight = displayMetrics.heightPixels;
    }

    public void changeDeviceName() {
        mDeviceName = "t" + mDeviceTail++;
    }

    @SuppressLint("SetTextI18n")
    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.btn_control: {
                if (!mIsStart) {
                    startServer();
                    mTxtDevice.setText("设备名称:" + mDeviceName);
                } else {
                    stopServer();
                    mTxtDevice.setText("未启动");
                }
                mIsStart = !mIsStart;
                mBtnControl.setText(mIsStart ? "结束" : "开始");
                break;
            }
        }
    }

    private void startServer() {
        changeDeviceName();
        mRaopServer.startServer(mDeviceName, mMacAddress);
        int raopPort = mRaopServer.getPort();
        if (raopPort == 0) {
            Toast.makeText(this.getApplicationContext(), "启动raop服务失败", Toast.LENGTH_SHORT).show();
        }
    }

    private void stopServer() {
        mRaopServer.stopServer();
    }

    public void processOutputFormat(int width, int height) {
        Message message = mHandler.obtainMessage(MSG_VIDEO_SIZE);
        message.arg1 = width;
        message.arg2 = height;
        mHandler.sendMessage(message);
    }

    public void updateTextureView(int width, int height) {
        Log.d(TAG, "updateTextureView width = " + width + ", height = " + height);
        RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) mTextureView.getLayoutParams();
        if (width < height) {
            // 缩放比例
            int scaleWidth = mScreenWidth;
            float scale = (float) scaleWidth / width;
            int scaleHeight = (int) (height * scale);
            // 竖屏
            params.width = scaleWidth;
            params.height = scaleHeight;
            if (scaleHeight < mScreenHeight) {
                params.topMargin = mScreenHeight - scaleHeight;
            }
            mTextureView.setLayoutParams(params);
        } else {
            // 横屏
            float rate = (float) width / height;
            int scaleWidth = mScreenWidth;
            int scaleHeight = (int) (scaleWidth / rate);
            params.width = scaleWidth;
            params.height = scaleHeight;
            params.topMargin = (mScreenHeight - scaleHeight) / 2;
            mTextureView.setLayoutParams(params);
        }
    }

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture surfaceTexture, int width, int height) {
        Log.d(TAG, "onSurfaceTextureAvailable width = " + width + ", height = " + height);
        mRaopServer.setSurface(new Surface(surfaceTexture));
    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture surfaceTexture, int width, int height) {
        Log.d(TAG, "onSurfaceTextureSizeChanged width = " + width + ", height = " + height);
    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surfaceTexture) {
        Log.d(TAG, "onSurfaceTextureDestroyed");
        return false;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surfaceTexture) {
        //Log.d(TAG, "onSurfaceTextureUpdated");
    }
}


