package no.nordicsemi.android.blinky.profile.callback;

import android.bluetooth.BluetoothDevice;
import android.graphics.Color;
import android.util.Log;

import androidx.annotation.NonNull;

import no.nordicsemi.android.ble.callback.DataSentCallback;
import no.nordicsemi.android.ble.callback.profile.ProfileDataCallback;
import no.nordicsemi.android.ble.data.Data;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

@SuppressWarnings("ConstantConditions")
public abstract class BlinkyBatteryDataCallback implements ProfileDataCallback, DataSentCallback, BlinkyBatteryCallback {
    @Override
    public void onDataReceived(@NonNull final BluetoothDevice
                                       device, @NonNull final Data data) {
        parse(device, data);
    }

    @Override
    public void onDataSent(@NonNull final BluetoothDevice device, @NonNull final Data data) {
        parse(device, data);
    }

    private void parse(@NonNull final BluetoothDevice device, @NonNull final Data data) {
        if (data.size() != 4) {
            onInvalidDataReceived(device, data);
            return;
        }

        final Float battery_state = ByteBuffer.wrap(data.getValue()).order(ByteOrder.LITTLE_ENDIAN).getFloat();
        onBatteryStateChanged(device, battery_state);
    }
}