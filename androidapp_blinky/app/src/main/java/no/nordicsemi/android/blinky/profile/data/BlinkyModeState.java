package no.nordicsemi.android.blinky.profile.data;

import androidx.annotation.NonNull;
import java.nio.ByteBuffer;
import no.nordicsemi.android.ble.data.Data;

public class BlinkyModeState {
    @NonNull
    public static Data setMode(final Integer state) {
        return Data.opCode(state.byteValue());
    }
}
