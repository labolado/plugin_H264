package com.plugin.h264;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

/**
 * H264 Decoder Service for Solar2D Plugin
 */
public class H264DecoderService extends Service {
    
    static {
        System.loadLibrary("plugin_h264");
    }
    
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
    
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return START_NOT_STICKY;
    }
    
    // Native methods
    public static native void initDecoder();
    public static native void cleanupDecoder();
    public static native int decodeFrame(byte[] data, int length);
}