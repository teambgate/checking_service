package com.bgate.nativeui;

import android.content.Context;
import android.graphics.Color;
import android.text.InputType;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.TextView;

import java.io.UnsupportedEncodingException;
import java.nio.charset.Charset;

/**
 * Created by apple on 5/25/17.
 */

public class CustomTextField extends CustomSharedView {

    public static CustomTextField create(long ptr, Context context) {
        CustomTextField p    = new CustomTextField(ptr, context);
        return p;
    }

    private InnerEditText content;

    public CustomTextField(long ptr, Context context) {
        super(ptr, context);

        content = new InnerEditText(context);
        content.setMaxLines(1);
        content.setInputType(InputType.TYPE_CLASS_TEXT);
        content.setHint("Input");

        LayoutParams params = new LayoutParams(
                LayoutParams.MATCH_PARENT,
                LayoutParams.MATCH_PARENT,
                0,
                0
        );
        content.setPadding(8, 8, 8, 8);
        content.setBackgroundColor(Color.TRANSPARENT);
        addViewInLayout(content, 0, params);
    }

    public String getText() {
        String text = null;
        try {
            text = content.getText().toString();
        } catch (Exception e) {}

        if(text == null) {
            text = "";
        }
        try {
            return new String(text.getBytes(), "UTF-8");
        } catch (UnsupportedEncodingException e) {
           return "";
        }
    }

    public void setPlaceholder(String text) {
        content.setHint(text);
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

//        @Override
//        public boolean onTouchEvent(MotionEvent event) {
//            if(can_touch == 1) {
//                return super.onTouchEvent(event);
//            } else {
//                return false;
//            }
//        }
        @Override
        public boolean onTouchEvent(MotionEvent event) {
            boolean result = false;
            if(can_touch == 1) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        result = super.onTouchEvent(event);
                        break;
                    case MotionEvent.ACTION_MOVE:
                        result = super.onTouchEvent(event);
                        break;
                    case MotionEvent.ACTION_UP:
                        result = super.onTouchEvent(event);
                        __current_touch_native_ptr__ = 0;
                        break;
                    case MotionEvent.ACTION_CANCEL:
                        result = super.onTouchEvent(event);
                        __current_touch_native_ptr__ = 0;
                        break;
                    default:
                        break;
                }
            }
            return result;
        }
    }

}
