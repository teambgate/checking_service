package com.bgate.nativeui;

import android.content.Context;
import android.graphics.Bitmap;
import android.support.v4.view.ViewCompat;
import android.view.MotionEvent;
import android.widget.AbsoluteLayout;
import android.widget.ImageView;

import com.example.apple.myapplication.R;

/**
 * Created by apple on 5/25/17.
 */

public class CustomImageView extends CustomSharedView {

    public static CustomImageView create(long ptr, Context context) {
        CustomImageView p    = new CustomImageView(ptr, context);
        return p;
    }

    private InnerImageView       content;

    public CustomImageView(long ptr, Context context) {
        super(ptr, context);

        content = new InnerImageView(context);
        content.setScaleType(ImageView.ScaleType.FIT_XY);
        AbsoluteLayout.LayoutParams params = new AbsoluteLayout.LayoutParams(
                AbsoluteLayout.LayoutParams.MATCH_PARENT,
                AbsoluteLayout.LayoutParams.MATCH_PARENT,
                0,
                0
        );
        addViewInLayout(content, 0, params);
    }

    public void set_bitmap(Bitmap bitmap)
    {
        content.setImageBitmap(bitmap);
    }

    public void load_image() {

    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        return false;
    }

    public class InnerImageView extends android.support.v7.widget.AppCompatImageView {

        public InnerImageView(Context context) {

            super(context);
//            ViewCompat.setLayerType(this, ViewCompat.LAYER_TYPE_HARDWARE, null);
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if(can_touch == 1) {
                float d  = getContext().getResources().getDisplayMetrics().density;
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        CustomFunction.touchBeganJNI(native_ptr, 0, event.getX() / d, event.getY() / d);
                        break;
                    case MotionEvent.ACTION_MOVE:
                        CustomFunction.touchMovedJNI(native_ptr, 0, event.getX() / d, event.getY() / d);
                        break;
                    case MotionEvent.ACTION_UP:
                        CustomFunction.touchEndedJNI(native_ptr, 0, event.getX() / d, event.getY() / d);
                        break;
                    case MotionEvent.ACTION_CANCEL:
                        CustomFunction.touchCancelledJNI(native_ptr, 0, event.getX() / d, event.getY() / d);
                        break;
                    default:
                        break;
                }
                return true;
            } else {
                return false;
            }
        }
    }
}
