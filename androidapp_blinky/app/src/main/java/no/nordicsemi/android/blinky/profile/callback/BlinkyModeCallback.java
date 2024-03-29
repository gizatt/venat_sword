package no.nordicsemi.android.blinky.profile.callback;

import android.bluetooth.BluetoothDevice;

import androidx.annotation.NonNull;

public interface BlinkyModeCallback {

    /**
     * Called when the data has been sent to the connected device.
     *
     * @param device the target device.
     * @param mode
     */
    void onModeStateChanged(@NonNull final BluetoothDevice device, final Integer mode);
}
