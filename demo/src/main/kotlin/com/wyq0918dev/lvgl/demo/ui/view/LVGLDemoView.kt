package com.wyq0918dev.lvgl.demo.ui.view

import android.content.Context
import android.graphics.SurfaceTexture
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.Surface
import android.view.TextureView

class LVGLDemoView(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : TextureView(context, attrs, defStyleAttr), TextureView.SurfaceTextureListener {

    private var mDestroyed: Boolean = false
    private var mSurface: Surface? = null

    init {
        surfaceTextureListener = this
        isOpaque = false
        isFocusable = true
        keepScreenOn = true
        isFocusableInTouchMode = true
        lvglCreate()
    }

    override fun onTouchEvent(event: MotionEvent?): Boolean {
        if (mDestroyed) return false
        event?.let {
            when (it.action) {
                MotionEvent.ACTION_DOWN, MotionEvent.ACTION_MOVE -> {
                    lvglOnTouch(
                        touch = 1,
                        x = it.x.toInt(),
                        y = it.y.toInt(),
                    )
                    return true
                }

                MotionEvent.ACTION_UP -> lvglOnTouch(
                    touch = 0,
                    x = it.x.toInt(),
                    y = it.y.toInt(),
                )

                MotionEvent.ACTION_CANCEL -> lvglOnTouch(
                    touch = 0,
                    x = it.x.toInt(),
                    y = it.y.toInt(),
                )
            }
        }
        if (event?.action == MotionEvent.ACTION_UP) {
            performClick()
        }
        return super.onTouchEvent(event)
    }

    override fun performClick(): Boolean {
        super.performClick()
        return false
    }

    override fun onSurfaceTextureAvailable(
        surface: SurfaceTexture,
        width: Int,
        height: Int
    ) {
        if (!mDestroyed) {
            mSurface = Surface(surface)
            lvglStart(mSurface)
        }
    }

    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture): Boolean {
        if (!mDestroyed) {
            lvglStop()
        }
        if (mSurface != null) {
            mSurface?.release()
            mSurface = null
        }
        return false
    }

    override fun onSurfaceTextureSizeChanged(
        surface: SurfaceTexture,
        width: Int,
        height: Int
    ) = Unit

    override fun onSurfaceTextureUpdated(
        surface: SurfaceTexture,
    ) = Unit

    override fun onDetachedFromWindow() {
        if (!mDestroyed) {
            lvglDestroy()
            mDestroyed = true
        }
        super.onDetachedFromWindow()
    }

    private external fun lvglCreate()
    private external fun lvglStart(surface: Surface?)
    private external fun lvglOnTouch(touch: Int, x: Int, y: Int)
    private external fun lvglStop()
    private external fun lvglDestroy()

    companion object {
        init {
            System.loadLibrary("lvgl_demo")
        }
    }
}