/*
 * Copyright (c) 2018, Nordic Semiconductor
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package no.nordicsemi.android.blinky;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Spinner;

import com.google.android.material.appbar.MaterialToolbar;

import androidx.appcompat.app.AppCompatActivity;
import androidx.lifecycle.ViewModelProvider;

import java.util.Timer;
import java.util.TimerTask;

import no.nordicsemi.android.ble.livedata.state.ConnectionState;
import no.nordicsemi.android.ble.observer.ConnectionObserver;
import no.nordicsemi.android.blinky.adapter.DiscoveredBluetoothDevice;
import no.nordicsemi.android.blinky.databinding.ActivityBlinkyBinding;
import no.nordicsemi.android.blinky.viewmodels.BlinkyViewModel;

public class BlinkyActivity extends AppCompatActivity implements AdapterView.OnItemSelectedListener {
	public static final String EXTRA_DEVICE = "no.nordicsemi.android.blinky.EXTRA_DEVICE";

	private BlinkyViewModel viewModel;
	private ActivityBlinkyBinding binding;

	@Override
	protected void onCreate(final Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		binding = ActivityBlinkyBinding.inflate(getLayoutInflater());
		setContentView(binding.getRoot());

		final Intent intent = getIntent();
		final DiscoveredBluetoothDevice device = intent.getParcelableExtra(EXTRA_DEVICE);
		final String deviceName = device.getName();
		final String deviceAddress = device.getAddress();

		final MaterialToolbar toolbar = binding.toolbar;
		toolbar.setTitle(deviceName != null ? deviceName : getString(R.string.unknown_device));
		toolbar.setSubtitle(deviceAddress);
		setSupportActionBar(toolbar);
		getSupportActionBar().setDisplayHomeAsUpEnabled(true);

		// Configure the view model.
		viewModel = new ViewModelProvider(this).get(BlinkyViewModel.class);
		viewModel.connect(device);

		// Set up views.
		binding.swordControls.onOffButton.setOnCheckedChangeListener((buttonView, isChecked) -> viewModel.setOnOffState(isChecked));
		binding.swordControls.colorPickerView.addOnColorChangedListener( (color) -> viewModel.setColor(color));

		Spinner spinner = binding.swordControls.modeSpinner;
		// Create an ArrayAdapter using the string array and a default spinner layout
		ArrayAdapter<CharSequence> adapter = ArrayAdapter.createFromResource(this,
				R.array.sword_modes_array, android.R.layout.simple_spinner_item);
		// Specify the layout to use when the list of choices appears
		adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		// Apply the adapter to the spinner
		spinner.setAdapter(adapter);
		spinner.setOnItemSelectedListener( this );
		binding.infoNotSupported.actionRetry.setOnClickListener(v -> viewModel.reconnect());
		binding.infoTimeout.actionRetry.setOnClickListener(v -> viewModel.reconnect());

		viewModel.getConnectionState().observe(this, state -> {
			switch (state.getState()) {
				case CONNECTING:
					binding.progressContainer.setVisibility(View.VISIBLE);
					binding.infoNotSupported.container.setVisibility(View.GONE);
					binding.infoTimeout.container.setVisibility(View.GONE);
					binding.connectionState.setText(R.string.state_connecting);
					break;
				case INITIALIZING:
					binding.connectionState.setText(R.string.state_initializing);
					break;
				case READY:
					binding.progressContainer.setVisibility(View.GONE);
					binding.deviceContainer.setVisibility(View.VISIBLE);
					onConnectionStateChanged(true);
					break;
				case DISCONNECTED:
					if (state instanceof ConnectionState.Disconnected) {
						binding.deviceContainer.setVisibility(View.GONE);
						binding.progressContainer.setVisibility(View.GONE);
						final ConnectionState.Disconnected stateWithReason = (ConnectionState.Disconnected) state;
						if (stateWithReason.getReason() == ConnectionObserver.REASON_NOT_SUPPORTED) {
							binding.infoNotSupported.container.setVisibility(View.VISIBLE);
						} else {
							binding.infoTimeout.container.setVisibility(View.VISIBLE);
						}
					}
					// fallthrough
				case DISCONNECTING:
					onConnectionStateChanged(false);
					break;
			}
		});
		viewModel.getOnOffState().observe(this, isOn ->
			binding.swordControls.onOffButton.setChecked(isOn)
		);
		viewModel.getColorState().observe(this,
				color ->
					binding.swordControls.colorPickerView.setColor(color, true) );
		viewModel.getBatteryState().observe(this,
				battery_state -> {
					binding.swordControls.batteryBar.setIndeterminate(false);
					binding.swordControls.batteryBarText.setText(String.format("%.2fV", battery_state));
					// Show minimum at 3V, maximum at 4.2V.
					binding.swordControls.batteryBar.setProgress((int) (100.0 * ( battery_state - 3.0) / (4.2 - 3.0))); });

		// Ugly hack to get constantly reported battery...
		Timer timer = new Timer();
		timer.scheduleAtFixedRate(new TimerTask() {
			public void run() {
				runOnUiThread(new Runnable() {
					public void run() {
						viewModel.updateBatteryState();
					}
				});
			}
		}, 1000, 1000);
	}

	private void onConnectionStateChanged(final boolean connected) {
		binding.swordControls.onOffButton.setEnabled(connected);
		binding.swordControls.brightnessSlider.setEnabled(connected);
		binding.swordControls.colorPickerView.setEnabled(connected);

		binding.swordControls.batteryBar.setEnabled(connected);
		if (!connected) {
			binding.swordControls.onOffButton.setChecked(false);
			binding.swordControls.batteryBar.setIndeterminate(true);
			binding.swordControls.batteryBarText.setText("???");
			// TODO: Set progress bar text to unknown.
		}
	}

	// Spinner callbacks.
	public void onItemSelected(AdapterView<?> parent, View view,
							   int pos, long id) {
		this.viewModel.setMode(pos);
	}

	public void onNothingSelected(AdapterView<?> parent) {
		this.viewModel.setMode(0);
	}
}
