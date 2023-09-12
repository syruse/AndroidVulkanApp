package com.android.myapp

import android.R
import android.annotation.SuppressLint
import android.os.Build.VERSION
import android.os.Build.VERSION_CODES
import android.os.Bundle
import android.view.KeyEvent
import android.view.View
import android.view.WindowManager.LayoutParams
import androidx.compose.runtime.snapshots.Snapshot.Companion.observe
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.lifecycle.Observer
import com.google.androidgamesdk.GameActivity
import kotlin.system.exitProcess


class VulkanActivity : GameActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        hideSystemUI()
        supportFragmentManager.beginTransaction().add(R.id.content, ControlsFragment()).commit()
        AppState.getHue().observe(this, Observer { hue ->
            applyFilterOverJNI(
                hue,
                AppState.getSaturation().value!!,
                AppState.getIntensity().value!!
            )
        })
        AppState.getSaturation().observe(this, Observer { saturation ->
            applyFilterOverJNI(
                AppState.getHue().value!!,
                saturation,
                AppState.getIntensity().value!!
            )
        })
        AppState.getIntensity().observe(this, Observer { intensity ->
            applyFilterOverJNI(
                AppState.getHue().value!!,
                AppState.getSaturation().value!!,
                intensity
            )
        })
    }

    private fun hideSystemUI() {
        // This will put the game behind any cutouts and waterfalls on devices which have
        // them, so the corresponding insets will be non-zero.

        // We cannot guarantee that AndroidManifest won't be tweaked
        // and we don't want to crash if that happens so we suppress warning.
        @SuppressLint("ObsoleteSdkInt")
        if (VERSION.SDK_INT >= VERSION_CODES.P) {
            window.attributes.layoutInDisplayCutoutMode =
                LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS
        }
        val decorView: View = window.decorView
        val controller = WindowInsetsControllerCompat(
            window,
            decorView
        )
        controller.hide(WindowInsetsCompat.Type.systemBars())
        controller.hide(WindowInsetsCompat.Type.displayCutout())
        controller.systemBarsBehavior =
            WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
    }

    // Filter out back button press, and handle it here after native
    // side done its processing. Application can also make a reverse JNI
    // call to onBackPressed()/finish() at the end of the KEYCODE_BACK
    // processing.
    override fun onKeyDown(keyCode: Int, event: KeyEvent?): Boolean {
        var processed = super.onKeyDown(keyCode, event);
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            onBackPressed()
            processed = true
        }
        return processed
    }

    override fun onBackPressed() {
        System.gc()
        exitProcess(0)
    }

    companion object {
        init {
            System.loadLibrary("my_engine")
        }
    }

    /**
     * A native method for image modifying over affecting HSV color system of original image,
     * factors are in range [0.0; 1.0] where 0.5 is default component
     */
    external fun applyFilterOverJNI(
        hueFactor: Float,
        saturationFacto: Float,
        intensityFacto: Float,
    ): Boolean
}