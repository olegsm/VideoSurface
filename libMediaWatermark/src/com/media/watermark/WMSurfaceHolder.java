
package com.media.watermark;

import android.annotation.SuppressLint;
import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.content.res.Resources;
import android.opengl.GLSurfaceView;
import android.util.DisplayMetrics;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.View;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.PixelFormat;

import java.nio.ByteBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

@SuppressLint("NewApi")
public class WMSurfaceHolder implements GLSurfaceView.Renderer {
    private GLSurfaceView mView;
    private SurfaceHolder.Callback mCallback;
    private int mWidth = 0;
    private int mHeight = 0;
    private SurfaceTextureHolder mHolder = null;
    private float[] mTransformMatrix = new float[16];
    private Bitmap mWatermark;

    private Runnable mCreatedCallback = new Runnable() {
        @Override
        public void run() {
            if (mCallback != null) {
                mCallback.surfaceCreated(mView.getHolder());
            }
        }
    };

    private Runnable mDestroyedCallback = new Runnable() {
        @Override
        public void run() {
            if (mCallback != null) {
                mCallback.surfaceDestroyed(mView.getHolder());
            }
        }
    };

    private Runnable mChangedCallback = new Runnable() {
        @Override
        public void run() {
            if (mCallback != null) {
                mCallback.surfaceChanged(mView.getHolder(), PixelFormat.RGB_565, mWidth, mHeight);
            }
        }
    };

    public static WMSurfaceHolder getHolder(Context context) {
        if (!isOpenGLES20Supported(context)) {
            return null;
        }
        return new WMSurfaceHolder(context);
    }

    public WMSurfaceHolder(Context context) {
        System.loadLibrary("MediaWatermark");

        mView = new GLSurfaceView(context);
        mView.setEGLContextClientVersion(2);
        // GLSurfaceView uses RGB_5_6_5 by default.
        mView.setEGLConfigChooser(8, 8, 8, 8, 8, 8);
        mView.setRenderer(this);
        mWatermark = BitmapFactory.decodeResource(context.getResources(), R.drawable.watermark);
    }

    public void addCallback(SurfaceHolder.Callback callback) {
        mCallback = callback;
    }

    public Surface getSurface() {
        return mHolder != null ? mHolder.surface() : null;
    }

    public void destroy() {
        native_release();
    }

    public View getView() {
        return mView;
    }

    private static boolean isOpenGLES20Supported(Context context) {
        ActivityManager am = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        ConfigurationInfo info = am.getDeviceConfigurationInfo();
        return (info.reqGlEsVersion >= 0x20000);
    }

    private void runInUIThread(Runnable runnable) {
        if (runnable != null && mView != null) {
            mView.post(runnable);
        }
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        if (mHolder != null) {
            mHolder.draw(mTransformMatrix);
        }
        native_draw(mTransformMatrix, mWidth, mHeight);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        mWidth = width;
        mHeight = height;
        runInUIThread(mChangedCallback);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        int w = Resources.getSystem().getDisplayMetrics().widthPixels;
        int h = Resources.getSystem().getDisplayMetrics().heightPixels;
        native_init(mWatermark, w, h);
        mHolder = new SurfaceTextureHolder(native_get_textureId());
        runInUIThread(mCreatedCallback);
    }

    private native void native_init(Bitmap watermark, int maxWidth, int maxHeight);
    private native int native_get_textureId();
    private native void native_release();
    private native void native_draw(float[] transformMatrix, int width, int height);
}
