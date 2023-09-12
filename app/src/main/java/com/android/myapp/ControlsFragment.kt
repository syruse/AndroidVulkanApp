package com.android.myapp

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.Text
import androidx.compose.runtime.getValue
import androidx.compose.runtime.livedata.observeAsState
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.ComposeView
import androidx.compose.ui.text.style.TextAlign
import androidx.fragment.app.Fragment

class ControlsFragment : Fragment() {

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View? {
        return ComposeView(requireContext()).apply {
            setContent {
                val sliderHuePosition by AppState.getHue().observeAsState(0.5f);
                val sliderSaturationPosition by AppState.getSaturation().observeAsState(0.5f);
                val sliderIntensityPosition by AppState.getIntensity().observeAsState(0.5f);
                Row(
                    verticalAlignment = Alignment.Top
                ) {
                    Column(modifier = Modifier.weight(1f)) {
                        Slider(
                            value = sliderHuePosition,
                            onValueChange = {
                                AppState.setHue(it)
                            }
                        )
                        Text(
                            modifier = Modifier.align(Alignment.CenterHorizontally),
                            style = MaterialTheme.typography.titleLarge,
                            text = "Hue ${String.format("%.1f", sliderHuePosition)}"
                        )
                    }
                    Column(modifier = Modifier.weight(1f)) {
                        Slider(
                            value = sliderSaturationPosition,
                            onValueChange = { AppState.setSaturation(it) }
                        )
                        Text(
                            modifier = Modifier.align(Alignment.CenterHorizontally),
                            style = MaterialTheme.typography.titleLarge,
                            text = "Saturation ${String.format("%.1f", sliderSaturationPosition)}"
                        )
                    }
                    Column(modifier = Modifier.weight(1f)) {
                        Slider(
                            value = sliderIntensityPosition,
                            onValueChange = { AppState.setIntensity(it) }
                        )
                        Text(
                            modifier = Modifier.align(Alignment.CenterHorizontally),
                            style = MaterialTheme.typography.titleLarge,
                            text = "Intensity ${String.format("%.1f", sliderIntensityPosition)}"
                        )
                    }
                }
                Row(
                    verticalAlignment = Alignment.Bottom
                ) {
                    Text(
                        text = "Before",
                        modifier = Modifier.weight(1f),
                        textAlign = TextAlign.Center,
                        style = MaterialTheme.typography.displayLarge
                    )
                    Text(
                        text = "After",
                        modifier = Modifier.weight(1f),
                        textAlign = TextAlign.Center,
                        style = MaterialTheme.typography.displayLarge,
                    )
                }
            }
        }
    }

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)
    }

}