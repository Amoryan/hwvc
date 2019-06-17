package com.lmy.samplenative

import android.graphics.SurfaceTexture
import android.os.Environment
import android.util.Log
import android.view.Surface
import android.view.SurfaceHolder
import android.view.TextureView
import com.lmy.hwvcnative.processor.PictureProcessor
import kotlinx.android.synthetic.main.activity_image.*

class ImageActivity : BaseActivity(), TextureView.SurfaceTextureListener {
    private lateinit var mFilterController: FilterController
    private var processor: PictureProcessor? = PictureProcessor()
    private var surface: Surface? = null
    private val surfaceCallback = object : SurfaceHolder.Callback {
        override fun surfaceChanged(holder: SurfaceHolder, p1: Int, p2: Int, p3: Int) {
            Log.i("HWVC", "surfaceChanged: $p1, $p2, $p3 | ${surfaceView.width}, ${surfaceView.height}")
            processor?.prepare(holder.surface, surfaceView.width, surfaceView.height)
            processor?.show("${Environment.getExternalStorageDirectory().path}/1.jpg")
        }

        override fun surfaceDestroyed(p0: SurfaceHolder?) {
            processor?.release()
            processor = null
            Log.i("HWVC", "surfaceDestroyed")
        }

        override fun surfaceCreated(holder: SurfaceHolder) {
            Log.i("HWVC", "surfaceCreated")
        }
    }

    override fun getLayoutResource(): Int = R.layout.activity_image
    override fun initView() {
        mFilterController = FilterController(processor!!, progressLayout)
        filterBtn.setOnClickListener {
            mFilterController.chooseFilter(this)
        }
        surfaceView.keepScreenOn = true
        surfaceView.holder.addCallback(surfaceCallback)
//        surfaceView.surfaceTextureListener = this
    }

    override fun onSurfaceTextureSizeChanged(surface: SurfaceTexture?, width: Int, height: Int) {
    }

    override fun onSurfaceTextureUpdated(surface: SurfaceTexture?) {
    }

    override fun onSurfaceTextureDestroyed(surface: SurfaceTexture?): Boolean {
//                processor?.release()
//                processor = null
        this.surface?.release()
        return true
    }

    override fun onSurfaceTextureAvailable(surface: SurfaceTexture, width: Int, height: Int) {
        this.surface = Surface(surface)
        processor?.prepare(this.surface!!, width, height)
        processor?.show("${Environment.getExternalStorageDirectory().path}/1.jpg")
    }

    override fun onDestroy() {
        super.onDestroy()
        surfaceView.holder.removeCallback(surfaceCallback)
    }
}