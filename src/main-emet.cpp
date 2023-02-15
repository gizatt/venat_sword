/**
 *  Control code for a Xiao BLE bluetooth controller controlling a few
 *  LED strips embedded in a cosplay prop.
 *
 *  This uC does a few things:
 *  - Controls a handful of NeoPixel LED strips.
 *  - Reads battery charge state, and disables all high-current functionality
 *    if the battery charge is below 3V. (Disabled on this prop.)
 *  - Runs a Bluetooth BLE server that:
 *     - Reads out the current battery voltage and control mode.
 *     - Enables control of LEDs.
 */

#include <Adafruit_NeoPixel.h>
#include "PropLEDDriver.h"
#include "PropBLEManager.h"
#include "StatusLEDManager.h"

// LED strip setup information
const int PIN_LEDS_UPPER = 10;
const int PIN_LEDS_LOWER = 8;
const int N_PIXELS = 3;

Adafruit_NeoPixel pixels_1 = Adafruit_NeoPixel(N_PIXELS, PIN_LEDS_UPPER, NEO_GRB);
Adafruit_NeoPixel pixels_2 = Adafruit_NeoPixel(N_PIXELS, PIN_LEDS_LOWER, NEO_GRB);

PropLEDDriver prop_led_driver;
StatusLEDManager status_led_manager(LED_BUILTIN);
PropBLEManager prop_ble_manager;

bool setup_leds()
{
  pixels_1.begin();
  pixels_2.begin();
  prop_led_driver.register_strips(&pixels_1, &pixels_2);
  return true;
}

bool setup_ble()
{
  prop_ble_manager.led_enabled = true;
  // Start in weak rainbow
  prop_ble_manager.led_rgb_setting_1[0] = 40;
  prop_ble_manager.led_rgb_setting_1[1] = 40;
  prop_ble_manager.led_rgb_setting_1[2] = 40;
  prop_ble_manager.led_rgb_setting_2[0] = 40;
  prop_ble_manager.led_rgb_setting_2[1] = 40;
  prop_ble_manager.led_rgb_setting_2[2] = 40;
  prop_ble_manager.control_mode = ControlMode::PartyModeFlowing;

  if (!prop_ble_manager.setup("Emet-Claymore"))
  {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    return false;
  }
  return true;
}

void setup()
{
  Serial.begin(9600);
  // Prep for battery voltage reading.
  analogReadResolution(12);
  status_led_manager.setup();

  // Flip LED 3 times if failed to setup LEDs.
  while (!setup_leds())
  {
    Serial.println("Failed to setup LEDs.");
    status_led_manager.flip_time_ms = 100;
    unsigned long start_t = millis();
    while (millis() - start_t < 1000)
    {
      status_led_manager.update();
    }
    delay(1000);
  }

  // Flip LED 5 times if failed to setup BLE.
  while (!setup_ble())
  {
    Serial.println("Failed to setup BLE.");
    status_led_manager.flip_time_ms = 250;
    unsigned long start_t = millis();
    while (millis() - start_t < 2000)
    {
      status_led_manager.update();
    }
    delay(1000);
  }
}

void loop()
{
  double t = ((double)millis()) / 1000.;

  // No battery to read.
  prop_ble_manager.update(true, 3.1415);
  prop_led_driver.update(
      {t,
       prop_ble_manager.led_enabled,
       {prop_ble_manager.led_rgb_setting_1[0], prop_ble_manager.led_rgb_setting_1[1], prop_ble_manager.led_rgb_setting_1[2]},
       prop_ble_manager.control_mode});

  if (prop_ble_manager.led_enabled)
  {
    status_led_manager.flip_time_ms = 250;
  }
  else
  {
    status_led_manager.flip_time_ms = 500;
  }
  status_led_manager.update();
}