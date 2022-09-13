package no.nordicsemi.android.blinky.profile.callback;

import android.bluetooth.BluetoothDevice;
import android.graphics.Color;
import android.util.Log;

import androidx.annotation.NonNull;

import no.nordicsemi.android.ble.callback.DataSentCallback;
import no.nordicsemi.android.ble.callback.profile.ProfileDataCallback;
import no.nordicsemi.android.ble.data.Data;

@SuppressWarnings("ConstantConditions")
public abstract class BlinkyColorDataCallback implements ProfileDataCallback, DataSentCallback, BlinkyColorCallback {
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
        if (data.size() != 3) {
            onInvalidDataReceived(device, data);
            return;
        }

        final int color = Color.argb(255, data.getByte(0), data.getByte(1), data.getByte(2));
        onColorStateChanged(device, color);
    }
}