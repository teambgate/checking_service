package com.bgate.nativeui;

import android.content.Context;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.inputmethod.EditorInfo;
import android.widget.TextView;

/**
 * Created by apple on 5/26/17.
 */

public class CustomTextView extends CustomSharedView {

    public static CustomTextView create(long ptr, Context context) {
        CustomTextView p    = new CustomTextView(ptr, context);
        return p;
    }

    private InnerEditText content;

    public CustomTextView(long ptr, Context context) {
        super(ptr, context);

        content = new InnerEditText(context);
        content.setLineSpacing(1, 1);

        LayoutParams params = new LayoutParams(
                LayoutParams.MATCH_PARENT,
                LayoutParams.MATCH_PARENT,
                0,
                0
        );
        addViewInLayout(content, 0, params);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        return false;
    }

    private class InnerEditText extends android.support.v7.widget.AppCompatEditText {

        public InnerEditText(Context context) {

            super(context);
            setImeOptions(EditorInfo.IME_ACTION_DONE);
            setOnEditorActionListener(new OnEditorActionListener() {
                @Override
                public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                    if (actionId == EditorInfo.IME_ACTION_DONE
                            || actionId == EditorInfo.IME_ACTION_SEARCH
                            || actionId == EditorInfo.IME_ACTION_GO
                            || actionId == EditorInfo.IME_ACTION_SEND) {
                        CustomFunction.textDoneJNI(native_ptr);
                    }

                    return false;
                }
            });
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if(can_touch == 1) {
                return super.onTouchEvent(event);
            } else {
                return false;
            }
        }
    }
}
