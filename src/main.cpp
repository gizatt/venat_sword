/**
 *  Control code for a Xiao BLE bluetooth controller controlling a few
 *  LED strips embedded in a cosplay prop.
 *
 *  This uC does a few things:
 *  - Controls a handful of NeoPixel LED strips.
 *  - Reads battery charge state, and disables all high-current functionality
 *    if the battery charge is below 3V.
 *  - Runs a Bluetooth BLE server that:
 *     - Reads out the current battery voltage and control mode.
 *     - Enables control of LEDs.
 */

#include <Adafruit_NeoPixel.h>
#include <ArduinoBLE.h>

// LED strip setup information
#define PIN 10
#define NUMPIXELS 100
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB);
bool led_enabled = true;
uint8_t led_rgb_setting[3] = {0, 0, 50}; // Start with soft blue color

// BLE service info
BLEService ble_service("198a8000-2ab7-414c-9459-47e3d418a7fd");
// Universal enable / disable toggle.
BLEBoolCharacteristic ble_switch_characteristic("198a8001-2ab7-414c-9459-47e3d418a7fd", BLERead | BLEWrite);
// RGB
BLECharacteristic ble_rgb_characteristic("198a8002-2ab7-414c-9459-47e3d418a7fd", BLERead | BLEWrite, 3, true);
// Battery state
BLEFloatCharacteristic ble_battery_characteristic("198a8003-2ab7-414c-9459-47e3d418a7fd", BLERead);

bool setup_leds()
{
  pixels.begin(); // This initializes the NeoPixel library.
  return true;
}

bool setup_ble()
{
  if (!BLE.begin())
  {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    return false;
  }
  // set advertised local name and service UUID:
  BLE.setLocalName("Venat");
  BLE.setAdvertisedService(ble_service);

  // Add characteristics.
  ble_service.addCharacteristic(ble_switch_characteristic);
  ble_service.addCharacteristic(ble_rgb_characteristic);
  ble_service.addCharacteristic(ble_battery_characteristic);

  // add service
  BLE.addService(ble_service);

  // set the initial value for on/off and rgb:
  ble_switch_characteristic.writeValue(led_enabled);
  ble_rgb_characteristic.writeValue(led_rgb_setting, 3);
  ble_battery_characteristic.writeValue(-1.23);

  // Prep for battery voltage reading.
  analogReadResolution(12);

  // start advertising
  BLE.advertise();

  return true;
}

int LED_STATE = 0;
unsigned long last_flip_time_ms = 0;
void flip_led()
{
  digitalWrite(LED_BUILTIN, LED_STATE);
  LED_STATE = !LED_STATE;
}

void setup()
{
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_STATE);

  // Flip LED 3 times if failed to setup LEDs.
  while (!setup_leds())
  {
    Serial.println("Failed to setup LEDs.");
    for (int i = 0; i < 3; i++)
    {
      flip_led();
      delay(250);
      flip_led();
      delay(250);
    }
  }

  // Flip LED 5 times if failed to setup BLE.
  while (!setup_ble())
  {
    Serial.println("Failed to setup BLE.");
    for (int i = 0; i < 5; i++)
    {
      flip_led();
      delay(250);
      flip_led();
      delay(250);
    }
  }
}

void loop()
{
  // Grab new device if available.
  BLEDevice central = BLE.central();

  // If a central is connected to peripheral:
  if (central && central.connected())
  {
    led_enabled = ble_switch_characteristic.value();
    memcpy(led_rgb_setting, ble_rgb_characteristic.value(), 3);
  }

  if (!led_enabled)
  {
    for (int i = 0; i < 3; i++)
    {
      led_rgb_setting[i] = 0;
    }
  }
  // For a set of NeoPixels the first NeoPixel is 0, second is 1, all the way up to the count of pixels minus one.
  for (int i = 0; i < NUMPIXELS; i++)
  {
    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(i, pixels.Color(led_rgb_setting[0], led_rgb_setting[1], led_rgb_setting[2]));
  }
  pixels.show(); // This sends the updated pixel color to the hardware.

  // Flip LED to show state.
  unsigned long flip_time_ms = led_enabled ? 250 : 1000;
  if (millis() - last_flip_time_ms > flip_time_ms)
  {
    last_flip_time_ms = millis();
    flip_led();
  }

  // Read the battery state and prepare it for publish.
  // The battery is in the middle of a voltage divider, so multiply
  // the read voltage accordingly:
  //   read voltage = bat_voltage * (TO_GND)/(TO_GND + TO_HOT)
  const float OHMS_TO_3V3 = 9910.0;
  const float OHMS_TO_GND = 9990.0;
  float read_voltage = 3.3 * ((float)analogRead(0)) / 4096.;
  ble_battery_characteristic.writeValue(read_voltage * (OHMS_TO_3V3 + OHMS_TO_GND) / (OHMS_TO_GND));
}