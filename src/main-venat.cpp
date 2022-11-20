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
const int SWORD_TIP_LED_START = 60;   // Number of LEDs along the strand where the rolled-back segments starts.
const int SWORD_TIP_LED_END = 150;    // Index of final LED in the strip.
const int SWORD_TIP_HALF_N_LEDS = 45; // Number of LEDs on one side of the rolled-back segment.

const int PIN_LEDS_GEMS = 8;
const int NUM_PIXELS_GEMS = 4;

const float MIN_BATTERY_VOLTAGE = 3.0;

Adafruit_NeoPixel pixels_gems = Adafruit_NeoPixel(NUM_PIXELS_GEMS, PIN_LEDS_GEMS, NEO_GRB);
Adafruit_NeoPixel pixels_sword = Adafruit_NeoPixel(SWORD_TIP_LED_END, PIN_LEDS_SWORD, NEO_GRB);

typedef enum ControlMode
{
  DirectRGB = 0,
  DirectRGBPulsing = 1,
  PartyModeFlowing = 2,
  PartyModeRolling = 3
} ControlMode;

bool led_enabled = true;
uint8_t led_rgb_setting_1[3] = {0, 0, 30}; // Start with soft blue color
uint8_t led_rgb_setting_2[3] = {0, 0, 30}; // Start with soft blue color
ControlMode control_mode = ControlMode::PartyModeFlowing;

// BLE service info
BLEService ble_service("198a8000-2ab7-414c-9459-47e3d418a7fd");
// Universal enable / disable toggle.
BLEBoolCharacteristic ble_switch_characteristic("198a8001-2ab7-414c-9459-47e3d418a7fd", BLERead | BLEWrite);
// Mode
BLEIntCharacteristic ble_mode_characteristic("198a8005-2ab7-414c-9459-47e3d418a7fd", BLERead | BLEWrite);
// RGB
BLECharacteristic ble_rgb_1_characteristic("198a8002-2ab7-414c-9459-47e3d418a7fd", BLERead | BLEWrite, 3, true);
BLECharacteristic ble_rgb_2_characteristic("198a8004-2ab7-414c-9459-47e3d418a7fd", BLERead | BLEWrite, 3, true);
// Battery state
BLEFloatCharacteristic ble_battery_characteristic("198a8003-2ab7-414c-9459-47e3d418a7fd", BLERead);

class SwordLEDDriver
{
public:
  // This struct maps 1-to-1 with state coming in from Bluetooth. It's a superset of
  // inputs needed for all control modes, plus some local state (battery level).
  typedef struct Color
  {
    uint8_t r;
    uint8_t g;
    uint8_t b;
  } Color;
  typedef struct ControlInput
  {
    double t; // Seconds, arbitrary 0 point.
    bool on_off;
    Color color_1;
    Color color_2;
  } ControlInput;

  SwordLEDDriver()
  {
  }

  /*
   Sets the i^th pixel along the sword blade to the given color.
   This function applies color corrections, handles the symmetric
   LED strips at the tip of the sword, and dims the last few pixels
   to not make the unlit tip look too relatively dim.
  */
  void setPixelColor(int i, uint8_t r, uint8_t g, uint8_t b)
  {
    if (i < SWORD_TIP_LED_START)
    {
      b *= 0.9;
      pixels_sword.setPixelColor(i, r, g, b);
    }
    else if (i >= SWORD_TIP_LED_START && i <= SWORD_TIP_LED_START + SWORD_TIP_HALF_N_LEDS)
    {
      // Apply color-correction for tip strip, and command symmetrically.
      // Current correction is to just slightly bump the blue.
      r *= 0.9;
      pixels_sword.setPixelColor(i, r, g, b);
      pixels_sword.setPixelColor(SWORD_TIP_LED_END - (i - SWORD_TIP_LED_START), r, g, b);
    }
  }

  void turn_off_all_leds()
  {
    for (int i = 0; i <= SWORD_TIP_LED_START + SWORD_TIP_HALF_N_LEDS * 2; i++)
    {
      pixels_sword.setPixelColor(i, 0, 0, 0);
    }
    for (int i = 0; i < NUM_PIXELS_GEMS; i++)
    {
      pixels_gems.setPixelColor(i, 0, 0, 0);
    }
    pixels_sword.show();
    pixels_gems.show();
  }

  void update_direct_rgb(ControlInput input)
  {
    // For a set of NeoPixels the first NeoPixel is 0, second is 1, all the way up to the count of pixels minus one.
    for (int i = 0; i <= SWORD_TIP_LED_START + SWORD_TIP_HALF_N_LEDS; i++)
    {
      setPixelColor(i, input.color_1.r, input.color_1.g, input.color_1.b);
    }
    for (int i = 0; i < NUM_PIXELS_GEMS; i++)
    {
      pixels_gems.setPixelColor(i, input.color_2.r, input.color_2.g, input.color_2.b);
    }
    pixels_sword.show();
    pixels_gems.show();
  }

  inline float get_pulsing_noise(float x, float t)
  {
    return ((cos(2 * x + t) * sin(x - 0.5 * t)) + 1) / 2.;
  }

  void update_direct_rgb_pulsing(ControlInput input)
  {
    // Same as direct_rgb, but apply a time-varying pulsing effect to make the sword look more
    // organic.
    const float dim_amount = 0.75;
    for (int i = 0; i <= SWORD_TIP_LED_START + SWORD_TIP_HALF_N_LEDS; i++)
    {
      float x = ((float)i) / 20.;
      float scale = 1. - dim_amount * get_pulsing_noise(x, input.t);
      setPixelColor(i, scale * input.color_1.r, scale * input.color_1.g, scale * input.color_1.b);
    }
    for (int i = 0; i < NUM_PIXELS_GEMS; i++)
    {
      float x = (float)i;
      float scale = 1. - dim_amount * get_pulsing_noise(x, input.t);
      pixels_gems.setPixelColor(i, scale * input.color_2.r, scale * input.color_2.g, scale * input.color_2.b);
    }
    pixels_sword.show();
    pixels_gems.show();
  }

  static inline Color get_rainbow(uint32_t hue, uint8_t value)
  {
    uint32_t c = pixels_sword.ColorHSV(hue, 255, value);
    c = pixels_sword.gamma32(c);
    return {(uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c};
  }

  void update_party_mode_flowing(ControlInput input)
  {
    // Use total RGB brightness but not colors.
    uint8_t value = sqrt(pow(input.color_1.r, 2) + pow(input.color_1.g, 2) + pow(input.color_1.b, 2.));
    // uint16_t hue = fmod(input.t * 10000., 65535);
    // uint32_t c = pixels_sword.ColorHSV(hue, 255, value);
    // uint8_t r = (uint8_t)(c >> 16);
    // uint8_t g = (uint8_t)(c >> 8);
    // uint8_t b = (uint8_t)c;
    // Blue is always slightly on; R and G cycle out of sync.
    for (int i = 0; i <= SWORD_TIP_LED_START + SWORD_TIP_HALF_N_LEDS; i++)
    {
      float x = i / 100. - 0.5 * input.t;
      uint8_t r = value * (cos(x * 1.) + 1.) / 2.;
      uint8_t g = value * (cos(x * 2) + 1.) / 2.;
      uint8_t b = value * (cos(x * 3) + 2.) / 3.;
      setPixelColor(i, r, g, b);
    }
    float x = 0 / 100. - 0.5 * input.t;
    uint8_t r = value * (cos(x * 1.) + 1.) / 2.;
    uint8_t g = value * (cos(x * 2) + 1.) / 2.;
    uint8_t b = value * (cos(x * 3) + 2.) / 3.;
    for (int i = 0; i < NUM_PIXELS_GEMS; i++)
    {
      pixels_gems.setPixelColor(i, r, g, b);
    }
    pixels_sword.show();
    pixels_gems.show();
  }
  void update_party_mode_rolling(ControlInput input)
  {
    // Use total RGB brightness but not colors.
    uint8_t value = sqrt(pow(input.color_1.r, 2) + pow(input.color_1.g, 2) + pow(input.color_1.b, 2.));
    // uint16_t hue = fmod(input.t * 40000., 65535);
    // uint32_t c = pixels_sword.ColorHSV(hue, 255, value);
    // c = pixels_sword.gamma32(c);
    // uint8_t r = (uint8_t)(c >> 16);
    // uint8_t g = (uint8_t)(c >> 8);
    // uint8_t b = (uint8_t)c;
    //  Blue is always slightly on; R and G cycle out of sync.
    uint8_t r = value * (cos(input.t) + 1.) / 2.;
    uint8_t g = value * (cos(input.t * 2) + 1.) / 2.;
    uint8_t b = value * (cos(input.t * 3) + 2.) / 3.;
    for (int i = 0; i <= SWORD_TIP_LED_START + SWORD_TIP_HALF_N_LEDS; i++)
    {
      setPixelColor(i, r, g, b);
    }
    for (int i = 0; i < NUM_PIXELS_GEMS; i++)
    {
      pixels_gems.setPixelColor(i, r, g, b);
    }
    pixels_sword.show();
    pixels_gems.show();
  }

  void update(ControlInput input)
  {
    if (!input.on_off)
    {
      turn_off_all_leds();
    }
    else
    {
      // Dispatch to mode-specific controller.
      switch (control_mode)
      {
      case ControlMode::DirectRGB:
        update_direct_rgb(input);
        break;
      case ControlMode::DirectRGBPulsing:
        update_direct_rgb_pulsing(input);
        break;
      case ControlMode::PartyModeFlowing:
        update_party_mode_flowing(input);
        break;
      case ControlMode::PartyModeRolling:
        update_party_mode_rolling(input);
        break;
      default:
        turn_off_all_leds();
        break;
      }
    }
  }
};
SwordLEDDriver sword_led_driver;

bool setup_leds()
{
  pixels_sword.begin();
  pixels_gems.begin();
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
  double t = ((double)millis()) / 1000.;

  // Grab new device if available.
  BLEDevice central = BLE.central();

  // If a central is connected to peripheral:
  if (central && central.connected())
  {
    led_enabled = ble_switch_characteristic.value();
    memcpy(led_rgb_setting_1, ble_rgb_1_characteristic.value(), 3);
    memcpy(led_rgb_setting_2, ble_rgb_2_characteristic.value(), 3);
    control_mode = (ControlMode)ble_mode_characteristic.value();
  }

  // Read the battery state and prepare it for publish.
  // The battery is in the middle of a voltage divider, so multiply
  // the read voltage accordingly:
  //   read voltage = bat_voltage * (TO_GND)/(TO_GND + TO_HOT)
  const float OHMS_TO_3V3 = 9910.0;
  const float OHMS_TO_GND = 9990.0;
  float read_voltage = 3.3 * ((float)analogRead(0)) / 4096.;
  float battery_voltage = read_voltage * (OHMS_TO_3V3 + OHMS_TO_GND) / (OHMS_TO_GND);
  ble_battery_characteristic.writeValue(battery_voltage);
  bool battery_dead = battery_voltage < MIN_BATTERY_VOLTAGE;
  if (battery_dead)
  {
    led_enabled = false;
    ble_switch_characteristic.writeValue(false);
  }

  sword_led_driver.update(
      {t,
       led_enabled,
       {led_rgb_setting_1[0], led_rgb_setting_1[1], led_rgb_setting_1[2]},
       {led_rgb_setting_2[0], led_rgb_setting_1[1], led_rgb_setting_2[2]}});

  // Flip LED to show state.
  // 5hz: battery dead
  // 2hz: leds on
  // 1hz: leds off, battery fine.
  unsigned long flip_time_ms;
  if (battery_dead)
  {
    flip_time_ms = 100;
  }
  else if (led_enabled)
  {
    flip_time_ms = 250;
  }
  else
  {
    flip_time_ms = 500;
  }
  if (millis() - last_flip_time_ms > flip_time_ms)
  {
    last_flip_time_ms = millis();
    flip_led();
  }
}