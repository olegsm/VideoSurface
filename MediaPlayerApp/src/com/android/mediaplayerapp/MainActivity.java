
package com.android.mediaplayerapp;

import com.media.watermark.WMSurfaceHolder;

import android.app.Activity;
import android.content.Context;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.FrameLayout;

public class MainActivity extends Activity {

    private FrameLayout mFrame;
    private VideoView mVideoView;
    private View mView;

    private String mUrl = "http://playertest.longtailvideo.com/adaptive/bbbfull/bbbfull.m3u8";

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        start();
    }

    @Override
    protected void onDestroy() {
        mVideoView.destroy();
        mVideoView = null;
        mFrame.removeView(mView);
        super.onDestroy();
    }

    private void start() {
        if (mVideoView == null) {
            Context context = getApplication();
            mFrame = new FrameLayout(context);

            mVideoView = new VideoView(context, mUrl);
            mView = mVideoView.getView();

            mFrame.addView(mView);
            setContentView(mFrame);
        }
    }

    public class VideoView extends SurfaceView implements SurfaceHolder.Callback,
            MediaPlayer.OnErrorListener {
        private SystemPlayer mPlayer;
        private String mUrl;
        private WMSurfaceHolder mHolder;

        public VideoView(Context context, final String url) {
            super(context);
            mUrl = url;

            mHolder = WMSurfaceHolder.getHolder(context);
            if (mHolder != null) {
                mHolder.addCallback(this);
            }

            if (mHolder == null) {
                SurfaceHolder holder = getHolder();
                if (holder != null) {
                    holder.addCallback(this);
                }
            }
        }

        public void destroy() {
            surfaceDestroyed(null);
        }

        public View getView() {
            return mHolder != null ? mHolder.getView() : this;
        }

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            mPlayer = new SystemPlayer(mUrl);
            mPlayer.start(mHolder != null ? mHolder.getSurface() : holder.getSurface());
        }

        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            if (mPlayer != null) {
                mPlayer.stop();
                mPlayer = null;
            }
            if (mHolder != null) {
                mHolder.destroy();
            }
        }

        @Override
        public boolean onError(MediaPlayer mp, int what, int extra) {
            surfaceDestroyed(null);
            surfaceCreated(null);
            return false;
        }
    }
}
