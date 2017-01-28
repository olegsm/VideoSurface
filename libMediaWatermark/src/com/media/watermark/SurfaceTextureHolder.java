package com.media.watermark;

import android.annotation.SuppressLint;
import android.graphics.SurfaceTexture;
import android.util.Log;
import android.view.Surface;

@SuppressLint("NewApi")
public class SurfaceTextureHolder {
    private static final String TAG = "SurfaceTextureHolder";
    private SurfaceTexture mSurfaceTexture;
    private Surface mSurface;

    private boolean mUpdateSurface = false;

    public SurfaceTextureHolder(int textureId) {
        mSurfaceTexture = new SurfaceTexture(textureId);
        mSurfaceTexture.setOnFrameAvailableListener(new SurfaceTexture.OnFrameAvailableListener() {
            @Override
            public void onFrameAvailable(SurfaceTexture texture) {
                synchronized (this) {
                    mUpdateSurface = true;
                }
            }
        });
    }

    public boolean draw(float[] transformMatrix) {
        synchronized (this) {
            //if (mUpdateSurface) {
                mSurfaceTexture.updateTexImage();
                if (transformMatrix != null) {
                    mSurfaceTexture.getTransformMatrix(transformMatrix);
                }
                mUpdateSurface = false;
                return true;
            //}
        }
        //return false;
    }

    public void release() {
        Log.v(TAG, "release");
        synchronized (this) {
            if (mSurface != null) {
                mSurface.release();
                mSurface = null;
            }
            if (mSurfaceTexture != null) {
                mSurfaceTexture.release();
                mSurfaceTexture = null;
            }
            mUpdateSurface = false;
        }
    }

    public SurfaceTexture texture() {
        return mSurfaceTexture;
    }

    public Surface surface() {
        if (mSurface == null && mSurfaceTexture != null) {
            mSurface = new Surface(mSurfaceTexture);
        }
        return mSurface;
    }
}

