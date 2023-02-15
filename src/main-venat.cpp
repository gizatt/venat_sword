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
#include "PropLEDDriver.h"
#include "PropBLEManager.h"
#include "StatusLEDManager.h"

/*

LED driver needs:

Strip wrapper that implements white balance and colors both sides
of the tip segment appropriately.

Fancy control modes, and a way to control them:
- Fixed RGB
- Subtle glowing / shifting / pulsing
- Rainbow mode


- Low battery flashing on gems
*/

// LED strip setup information
const int PIN_LEDS_SWORD = 10;
const int PIN_LEDS_GEMS = 8;
const int NUM_PIXELS_GEMS = 4;
const float MIN_BATTERY_VOLTAGE = 3.0;

const int SWORD_TIP_LED_START = 60;   // Number of LEDs along the strand where the rolled-back segments starts.
const int SWORD_TIP_LED_END = 150;    // Index of final LED in the strip.
const int SWORD_TIP_HALF_N_LEDS = 45; // Number of LEDs on one side of the rolled-back segment.
Adafruit_NeoPixel pixels_gems = Adafruit_NeoPixel(NUM_PIXELS_GEMS, PIN_LEDS_GEMS, NEO_GRB);
Adafruit_NeoPixel pixels_sword = Adafruit_NeoPixel(SWORD_TIP_LED_END, PIN_LEDS_SWORD, NEO_GRB);

class SwordLEDDriver : public PropLEDDriver
{
public:
  /*
   Sets the i^th pixel along the sword blade to the given color.
   This function applies color corrections, handles the symmetric
   LED strips at the tip of the sword, and dims the last few pixels
   to not make the unlit tip look too relatively dim.
  */
  inline void setPixels1Color(int i, uint8_t r, uint8_t g, uint8_t b) override
  {
    if (i < SWORD_TIP_LED_START)
    {
      b *= 0.9;
      m_pixels_1->setPixelColor(i, r, g, b);
    }
    else if (i >= SWORD_TIP_LED_START && i <= SWORD_TIP_LED_START + SWORD_TIP_HALF_N_LEDS)
    {
      // Apply color-correction for tip strip, and command symmetrically.
      // Current correction is to just slightly bump the blue.
      r *= 0.9;
      m_pixels_1->setPixelColor(i, r, g, b);
      m_pixels_1->setPixelColor(SWORD_TIP_LED_END - (i - SWORD_TIP_LED_START), r, g, b);
    }
    // Ignores pixels above halfway up the tip.
  }
};

SwordLEDDriver sword_led_driver;
StatusLEDManager status_led_manager(LED_BUILTIN);
PropBLEManager prop_ble_manager;

bool setup_leds()
{
  pixels_sword.begin();
  pixels_gems.begin();
  sword_led_driver.register_strips(&pixels_sword, &pixels_gems);
  return true;
}

bool setup_ble()
{
  prop_ble_manager.led_enabled = true;
  // Start soft blue
  prop_ble_manager.led_rgb_setting_1[0] = 20;
  prop_ble_manager.led_rgb_setting_1[1] = 20;
  prop_ble_manager.led_rgb_setting_1[2] = 30;
  prop_ble_manager.led_rgb_setting_2[0] = 20;
  prop_ble_manager.led_rgb_setting_2[1] = 20;
  prop_ble_manager.led_rgb_setting_2[2] = 30;
  prop_ble_manager.control_mode = ControlMode::DirectRGBPulsing;

  if (!prop_ble_manager.setup("Venat-Sword"))
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

  // Read the battery state and prepare it for publish.
  // The battery is in the middle of a voltage divider, so multiply
  // the read voltage accordingly:
  //   read voltage = bat_voltage * (TO_GND)/(TO_GND + TO_HOT)
  const float OHMS_TO_3V3 = 9910.0;
  const float OHMS_TO_GND = 9990.0;
  float read_voltage = 3.3 * ((float)analogRead(0)) / 4096.;
  float battery_voltage = read_voltage * (OHMS_TO_3V3 + OHMS_TO_GND) / (OHMS_TO_GND);
  bool battery_dead = battery_voltage < MIN_BATTERY_VOLTAGE;
  if (battery_dead)
  {
    prop_ble_manager.update(true, battery_voltage);
  }
  else
  {
    prop_ble_manager.update(false, battery_voltage);
  }

  sword_led_driver.update(
      {t,
       prop_ble_manager.led_enabled,
       {prop_ble_manager.led_rgb_setting_1[0], prop_ble_manager.led_rgb_setting_1[1], prop_ble_manager.led_rgb_setting_1[2]},
       prop_ble_manager.control_mode});

  // Flip LED to show state.
  // 5hz: battery dead
  // 2hz: leds on
  // 1hz: leds off, battery fine.
  if (battery_dead)
  {
    status_led_manager.flip_time_ms = 100;
  }
  else if (prop_ble_manager.led_enabled)
  {
    status_led_manager.flip_time_ms = 250;
  }
  else
  {
    status_led_manager.flip_time_ms = 500;
  }
  status_led_manager.update();
}