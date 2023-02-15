#pragma once

#include <ArduinoBLE.h>

typedef enum ControlMode
{
    DirectRGB = 0,
    DirectRGBPulsing = 1,
    PartyModeFlowing = 2,
    PartyModeRolling = 3
} ControlMode;

class PropBLEManager
{
public:
    bool led_enabled = false;
    uint8_t led_rgb_setting_1[3] = {0, 0, 0};
    uint8_t led_rgb_setting_2[3] = {0, 0, 0}; // unused
    ControlMode control_mode = ControlMode::DirectRGB;

    // BLE service info
    BLEService ble_service;
    // Universal enable / disable toggle.
    BLEBoolCharacteristic ble_switch_characteristic;
    // Mode
    BLEIntCharacteristic ble_mode_characteristic;
    // RGB
    BLECharacteristic ble_rgb_1_characteristic;
    BLECharacteristic ble_rgb_2_characteristic; // unused
    // Battery state
    BLEFloatCharacteristic ble_battery_characteristic;

    PropBLEManager() : ble_service("198a8000-2ab7-414c-9459-47e3d418a7fd"),
                       ble_switch_characteristic("198a8001-2ab7-414c-9459-47e3d418a7fd", BLERead | BLEWrite),
                       ble_mode_characteristic("198a8005-2ab7-414c-9459-47e3d418a7fd", BLERead | BLEWrite),
                       ble_rgb_1_characteristic("198a8002-2ab7-414c-9459-47e3d418a7fd", BLERead | BLEWrite, 3, true),
                       ble_rgb_2_characteristic("198a8004-2ab7-414c-9459-47e3d418a7fd", BLERead | BLEWrite, 3, true),
                       ble_battery_characteristic("198a8003-2ab7-414c-9459-47e3d418a7fd", BLERead)

    {
    }

    bool setup(const char *name)
    {
        if (!BLE.begin())
        {
            return false;
        }
        // set advertised local name and service UUID:
        BLE.setLocalName(name);
        BLE.setAdvertisedService(ble_service);

        // Add characteristics.
        ble_service.addCharacteristic(ble_switch_characteristic);
        ble_service.addCharacteristic(ble_rgb_1_characteristic);
        ble_service.addCharacteristic(ble_rgb_2_characteristic);
        ble_service.addCharacteristic(ble_battery_characteristic);
        ble_service.addCharacteristic(ble_mode_characteristic);

        // add service
        BLE.addService(ble_service);

        // set the initial value for on/off and rgb:
        ble_switch_characteristic.writeValue(led_enabled);
        ble_rgb_1_characteristic.writeValue(led_rgb_setting_1, 3);
        ble_rgb_2_characteristic.writeValue(led_rgb_setting_2, 3);
        ble_battery_characteristic.writeValue(-1.23);
        ble_mode_characteristic.writeValue(control_mode);
        // start advertising
        BLE.advertise();

        return true;
    }

    void update(bool force_led_disabled, float battery_voltage)
    {
        // Grab new device if available.
        BLEDevice central = BLE.central();

        if (central && central.connected())
        {
            led_enabled = ble_switch_characteristic.value();
            memcpy(led_rgb_setting_1, ble_rgb_1_characteristic.value(), 3);
            memcpy(led_rgb_setting_2, ble_rgb_2_characteristic.value(), 3);
            control_mode = (ControlMode)ble_mode_characteristic.value();
        }
        if (force_led_disabled)
        {
            ble_switch_characteristic.writeValue(led_enabled);
        }
        ble_battery_characteristic.writeValue(battery_voltage);
    }
};
