package no.nordicsemi.android.blinky.profile.data;

import android.graphics.Color;

import androidx.annotation.NonNull;

import java.nio.ByteBuffer;

import no.nordicsemi.android.ble.data.Data;

public final class BlinkyBatteryState {
    @NonNull
    public static Data setColor(final Float battery_state) {
        return new Data(ByteBuffer.allocate(4).putFloat(battery_state).array());
    }
}