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

package no.nordicsemi.android.blinky.profile;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.content.Context;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import java.util.UUID;

import no.nordicsemi.android.ble.data.Data;
import no.nordicsemi.android.ble.livedata.ObservableBleManager;
import no.nordicsemi.android.blinky.BuildConfig;
import no.nordicsemi.android.blinky.profile.callback.BlinkyBatteryDataCallback;
import no.nordicsemi.android.blinky.profile.callback.BlinkyColorDataCallback;
import no.nordicsemi.android.blinky.profile.callback.BlinkyOnOffDataCallback;
import no.nordicsemi.android.blinky.profile.data.BlinkyColorState;
import no.nordicsemi.android.blinky.profile.data.BlinkyOnOffState;
import no.nordicsemi.android.log.LogContract;
import no.nordicsemi.android.log.LogSession;
import no.nordicsemi.android.log.Logger;

public class BlinkyManager extends ObservableBleManager {
	/** Nordic Blinky Service UUID. */
	public final static UUID LBS_UUID_SERVICE = UUID.fromString("198a8000-2ab7-414c-9459-47e3d418a7fd");
	// Services
	private final static UUID LBS_UUID_ONOFF_CHAR = UUID.fromString("198a8001-2ab7-414c-9459-47e3d418a7fd");
	private final static UUID LBS_UUID_COLOR_CHAR = UUID.fromString("198a8002-2ab7-414c-9459-47e3d418a7fd");
	private final static UUID LBS_UUID_BATTERY_CHAR = UUID.fromString("198a8003-2ab7-414c-9459-47e3d418a7fd");
	// TODO: Add battery state.

	private final MutableLiveData<Boolean> onOffState = new MutableLiveData<>();
	private final MutableLiveData<Float> batteryState = new MutableLiveData<>();
	private final MutableLiveData<Integer> colorState = new MutableLiveData<>();

	private BluetoothGattCharacteristic colorCharacteristic, onOffCharacteristic, batteryCharacteristic;
	private LogSession logSession;
	private boolean supported;

	public BlinkyManager(@NonNull final Context context) {
		super(context);
	}

	public final LiveData<Boolean> getOnOffState() {
		return onOffState;
	}
	public final LiveData<Float> getBatteryState() {
		return batteryState;
	}
	public final LiveData<Integer> getColorState() {
		return colorState;
	}

	@NonNull
	@Override
	protected BleManagerGattCallback getGattCallback() {
		return new BlinkyBleManagerGattCallback();
	}

	/**
	 * Sets the log session to be used for low level logging.
	 * @param session the session, or null, if nRF Logger is not installed.
	 */
	public void setLogger(@Nullable final LogSession session) {
		logSession = session;
	}

	@Override
	public void log(final int priority, @NonNull final String message) {
		if (BuildConfig.DEBUG) {
			Log.println(priority, "BlinkyManager", message);
		}
		// The priority is a Log.X constant, while the Logger accepts it's log levels.
		Logger.log(logSession, LogContract.Log.Level.fromPriority(priority), message);
	}

	@Override
	protected boolean shouldClearCacheWhenDisconnected() {
		return !supported;
	}

	/**
	 * The on/off callback will be notified when the on/off state was read or sent to the target device.
	 */
	private final BlinkyOnOffDataCallback onOffCallback = new BlinkyOnOffDataCallback() {
		@SuppressLint("WrongConstant")
		@Override
		public void onOnOffStateChanged(@NonNull final BluetoothDevice device,
									  final boolean on) {
			log(LogContract.Log.Level.APPLICATION, "ON/OFF " + (on ? "ON" : "OFF"));
			// The BlinkyManager is initialized with a default Handler, which will use
			// UI thread for the callbacks. setValue can be called safely.

			// If you're using a different handler, or coroutines, use postValue(..) instead.
			onOffState.setValue(on);
		}

		@Override
		public void onInvalidDataReceived(@NonNull final BluetoothDevice device,
										  @NonNull final Data data) {
			// Data can only invalid if we read them. We assume the app always sends correct data.
			log(Log.WARN, "Invalid data received: " + data);
		}
	};

	/**
	 * The color callback will be notified when the color state was read or sent to the target device.
	 */
	private final BlinkyColorDataCallback colorCallback = new BlinkyColorDataCallback() {
		@SuppressLint("WrongConstant")
		@Override
		public void onColorStateChanged(@NonNull final BluetoothDevice device,
										final Integer color) {
			log(LogContract.Log.Level.APPLICATION, "Color " + color);
			colorState.setValue(color);
		}

		@Override
		public void onInvalidDataReceived(@NonNull final BluetoothDevice device,
										  @NonNull final Data data) {
			// Data can only invalid if we read them. We assume the app always sends correct data.
			log(Log.WARN, "Invalid data received: " + data);
		}
	};

	/**
	 * The battery callback will be notified when the battery state was read or sent to the target device.
	 */
	private final BlinkyBatteryDataCallback batteryCallback = new BlinkyBatteryDataCallback() {
		@SuppressLint("WrongConstant")
		@Override
		public void onBatteryStateChanged(@NonNull final BluetoothDevice device,
										final Float battery_state) {
			log(LogContract.Log.Level.APPLICATION, "Battery state " + battery_state);
			batteryState.setValue(battery_state);
		}

		@Override
		public void onInvalidDataReceived(@NonNull final BluetoothDevice device,
										  @NonNull final Data data) {
			// Data can only invalid if we read them. We assume the app always sends correct data.
			log(Log.WARN, "Invalid data received: " + data);
		}
	};

	/**
	 * BluetoothGatt callbacks object.
	 */
	private class BlinkyBleManagerGattCallback extends BleManagerGattCallback {
		@Override
		protected void initialize() {
			setNotificationCallback(batteryCharacteristic).with(batteryCallback);
			readCharacteristic(onOffCharacteristic).with(onOffCallback).enqueue();
			readCharacteristic(colorCharacteristic).with(colorCallback).enqueue();
			readCharacteristic(batteryCharacteristic).with(batteryCallback).enqueue();
			enableNotifications(batteryCharacteristic).enqueue();
		}

		@Override
		public boolean isRequiredServiceSupported(@NonNull final BluetoothGatt gatt) {
			final BluetoothGattService service = gatt.getService(LBS_UUID_SERVICE);
			if (service != null) {
				onOffCharacteristic = service.getCharacteristic(LBS_UUID_ONOFF_CHAR);
				colorCharacteristic = service.getCharacteristic(LBS_UUID_COLOR_CHAR);
				batteryCharacteristic = service.getCharacteristic(LBS_UUID_BATTERY_CHAR);
			}

			boolean onOffWriteRequest = false;
			if (onOffCharacteristic != null) {
				final int onOffProperties = onOffCharacteristic.getProperties();
				onOffWriteRequest = (onOffProperties & BluetoothGattCharacteristic.PROPERTY_WRITE) > 0;
				log(Log.VERBOSE, "onOff writeRequest " + onOffWriteRequest);
			}

			boolean colorWriteRequest = false;
			if (colorCharacteristic != null) {
				final int colorProperties = colorCharacteristic.getProperties();
				colorWriteRequest = (colorProperties & BluetoothGattCharacteristic.PROPERTY_WRITE) > 0;
				log(Log.VERBOSE, "color writeRequest " + colorWriteRequest);
			}

			supported = colorCharacteristic != null && onOffCharacteristic != null && batteryCharacteristic != null && onOffWriteRequest && colorWriteRequest;
			log(Log.VERBOSE, "supported " + supported);
			return supported;
		}

		@Override
		protected void onServicesInvalidated() {
			colorCharacteristic = null;
			onOffCharacteristic = null;
			batteryCharacteristic = null;
		}
	}

	/**
	 * Sends a request to the device to turn the LED on or off.
	 *
	 * @param on true to turn the LED on, false to turn it off.
	 */
	public void turnOnOff(final boolean on) {
		// Are we connected?
		if (onOffCharacteristic == null)
			return;

		log(Log.VERBOSE, "Turning LED " + (on ? "ON" : "OFF") + "...");
		writeCharacteristic(
				onOffCharacteristic,
				BlinkyOnOffState.turn(on),
				BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
		).with(onOffCallback).enqueue();
	}

	/**
	 * Sends a request to the device to change the color.
	 */
	public void setColor(final Integer color) {
		// Are we connected?
		if (colorCharacteristic == null)
			return;

		log(Log.VERBOSE, "Sending color " + color);
		writeCharacteristic(
				colorCharacteristic,
				BlinkyColorState.setColor(color),
				BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
		).with(colorCallback).enqueue();
	}

	public void updateBatteryState() {
		readCharacteristic(batteryCharacteristic).with(batteryCallback).enqueue();
	}
}
