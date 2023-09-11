package com.android.myapp

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData

object AppState {
    private val mHue = MutableLiveData<Float>(0.5f)
    private val mSaturation = MutableLiveData<Float>(0.5f)
    private val mIntensity = MutableLiveData<Float>(0.5f)

    fun setHue(hue: Float) {
        mHue.postValue(hue)
    }

    fun getHue(): LiveData<Float> {
        return mHue
    }

    fun setSaturation(saturation: Float) {
        mSaturation.postValue(saturation)
    }

    fun getSaturation(): LiveData<Float> {
        return mSaturation
    }

    fun setIntensity(intensity: Float) {
        mIntensity.postValue(intensity)
    }

    fun getIntensity(): LiveData<Float> {
        return mIntensity
    }
}