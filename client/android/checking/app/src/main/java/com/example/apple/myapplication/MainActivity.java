package com.example.apple.myapplication;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.Display;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.AbsoluteLayout;
import android.widget.TextView;

import com.bgate.nativeui.CustomView;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    private int mInterval = 16;
    private Handler mHandler;
    private boolean resized = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Display display = getWindowManager().getDefaultDisplay();
        int width = display.getWidth();
        int height = display.getHeight();

        setContentView(R.layout.activity_main);
        final AbsoluteLayout layout = (AbsoluteLayout) findViewById(R.id.root);

//        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
//            Window w = getWindow(); // in Activity's onCreate() for instance
//            w.setFlags(WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS, WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS);
//            View decor = getWindow().getDecorView();
//            decor.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR);
//        }

        final CustomView root = initNative();

        AbsoluteLayout.LayoutParams params = new AbsoluteLayout.LayoutParams(
                AbsoluteLayout.LayoutParams.MATCH_PARENT,
                AbsoluteLayout.LayoutParams.MATCH_PARENT,
                0,
                0
        );
        layout.addView(root, params);
        layout.forceLayout();

        mHandler = new Handler(Looper.getMainLooper());
        startRepeatingTask();

        root.post(new Runnable() {
            @Override
            public void run() {
                float d   = getResources().getDisplayMetrics().density;
                onResizeJNI((int)(root.getWidth() / d), (int)(root.getHeight() / d));
                resized = true;
            }
        });
    }

    private CustomView initNative() {
        String apkFilePath = null;

        ApplicationInfo appInfo = null;
        PackageManager packMgmr = getPackageManager();
        try {
            appInfo = packMgmr.getApplicationInfo(getPackageName(), 0);
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
            throw new RuntimeException("Unable to locate assets, aborting...");
        }
        apkFilePath = appInfo.sourceDir;
        return initNativeJNI(apkFilePath, getFilesDir().getAbsolutePath(), this);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        stopRepeatingTask();
    }

    Runnable mStatusChecker = new Runnable() {
        @Override
        public void run() {
            try {
                if(resized) onLoopJNI();
            } finally {
                mHandler.postDelayed(mStatusChecker, mInterval);
            }
        }
    };

    void startRepeatingTask() {
        mStatusChecker.run();
    }

    void stopRepeatingTask() {
        mHandler.removeCallbacks(mStatusChecker);
    }

    /* JNI */
    public native CustomView initNativeJNI(String asset, String local, Context context);

    public native void onResizeJNI(int width, int height);

    public native void onLoopJNI();
}
