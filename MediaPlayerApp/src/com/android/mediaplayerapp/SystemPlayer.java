package com.android.mediaplayerapp;

import android.media.MediaPlayer;
import android.view.Surface;

import java.io.IOException;

public class SystemPlayer implements MediaPlayer.OnPreparedListener {

    private MediaPlayer mPlayer = null;
    private String mUrl;

    public SystemPlayer(final String url) {
        mUrl = url;
    }

    public void stop() {
        if (mPlayer != null) {
            if (mPlayer.isPlaying()) {
                mPlayer.stop();
            }
            mPlayer.reset();
            mPlayer.release();
            mPlayer = null;
        }
    }

    public void start(Surface surface) {
        if (surface == null) {
            return;
        }

        try {
            mPlayer = new MediaPlayer();
            mPlayer.setOnPreparedListener(this);

            mPlayer.setSurface(surface);
            mPlayer.setScreenOnWhilePlaying(true);
            mPlayer.setDataSource(mUrl);
            mPlayer.setLooping(true);
            mPlayer.prepareAsync();
        } catch (IOException e) {
        }
    }

    @Override
    public void onPrepared(MediaPlayer mp) {
        if (mPlayer != null) {
            mPlayer.start();
        }
    }
}
