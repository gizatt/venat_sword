package no.nordicsemi.android.blinky.profile.data;

import android.graphics.Color;

import androidx.annotation.NonNull;

import no.nordicsemi.android.ble.data.Data;

public final class BlinkyColorState {
    @NonNull
    public static Data setColor(final Integer color) {
        return new Data(new byte[] {(byte) Color.red(color), (byte)Color.green(color), (byte)Color.blue(color)});
    }
}