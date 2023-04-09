#pragma once

#include <Adafruit_NeoPixel.h>
#include "PropBLEManager.h"

class PropLEDDriver
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
    Color color;
    ControlMode control_mode;
  } ControlInput;

  Adafruit_NeoPixel *m_pixels_1;
  Adafruit_NeoPixel *m_pixels_2;
  PropLEDDriver()
  {
  }

  // Hacky support to "grow" into a new mode
  // by updating more and more pixels as the mode
  // begins.
  unsigned long m_last_mode_change_ms = 0;
  ControlMode m_last_control_mode;
  unsigned long get_millis_since_last_mode_change()
  {
    return millis() - m_last_mode_change_ms;
  }
  const int MS_PER_PIXEL = 25;
  int get_num_leds_to_update(const Adafruit_NeoPixel& pixels){
    int num_to_update = get_millis_since_last_mode_change() / MS_PER_PIXEL;
    return max(min(num_to_update, pixels.numPixels()), 0);
  }

  void register_strips(Adafruit_NeoPixel *pixels_1, Adafruit_NeoPixel *pixels_2)
  {
    m_pixels_1 = pixels_1;
    m_pixels_2 = pixels_2;
  }

  // Overload these for special handling, e.g. sword blade. Assumes relevant strip exists.
  virtual inline void setPixels1Color(int i, uint8_t r, uint8_t g, uint8_t b)
  {
    m_pixels_1->setPixelColor(i, r, g, b);
  }
  virtual inline void setPixels2Color(int i, uint8_t r, uint8_t g, uint8_t b)
  {
    m_pixels_2->setPixelColor(i, r, g, b);
  }

  void turn_off_all_leds()
  {
    if (m_pixels_1)
    {
      for (int i = 0; i <= m_pixels_1->numPixels(); i++)
      {
        setPixels1Color(i, 0, 0, 0);
      }
      m_pixels_1->show();
    }
    if (m_pixels_2)
    {
      for (int i = 0; i <= m_pixels_2->numPixels(); i++)
      {
        setPixels2Color(i, 0, 0, 0);
      }
      m_pixels_2->show();
    }
  }

  void update_direct_rgb(ControlInput input)
  {
    if (m_pixels_1)
    {
      for (int i = 0; i <= get_num_leds_to_update(*m_pixels_1); i++)
      {
        setPixels1Color(i, input.color.r, input.color.g, input.color.b);
      }
      m_pixels_1->show();
    }
    if (m_pixels_2)
    {
      for (int i = 0; i <= get_num_leds_to_update(*m_pixels_2); i++)
      {
        setPixels2Color(i, input.color.r, input.color.g, input.color.b);
      }
      m_pixels_2->show();
    }
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

    if (m_pixels_1)
    {
      for (int i = 0; i <= get_num_leds_to_update(*m_pixels_1); i++)
      {

        float x = ((float)i) / 20.;
        float scale = 1. - dim_amount * get_pulsing_noise(x, input.t);
        setPixels1Color(i, scale * input.color.r, scale * input.color.g, scale * input.color.b);
      }
      m_pixels_1->show();
    }
    if (m_pixels_2)
    {
      for (int i = 0; i <= get_num_leds_to_update(*m_pixels_2); i++)
      {
        float x = ((float)i) / 20.;
        float scale = 1. - dim_amount * get_pulsing_noise(x, input.t);
        setPixels2Color(i, scale * input.color.r, scale * input.color.g, scale * input.color.b);
      }
      m_pixels_2->show();
    }
  }

  inline Color get_rainbow(uint32_t hue, uint8_t value)
  {
    uint32_t c = m_pixels_1->ColorHSV(hue, 255, value);
    c = m_pixels_1->gamma32(c);
    return {(uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c};
  }

  void update_party_mode_flowing(ControlInput input)
  {
    // Use total RGB brightness but not colors.
    uint8_t value = sqrt(pow(input.color.r, 2) + pow(input.color.g, 2) + pow(input.color.b, 2.));
    // uint16_t hue = fmod(input.t * 10000., 65535);
    // uint32_t c = pixels_sword.ColorHSV(hue, 255, value);
    // uint8_t r = (uint8_t)(c >> 16);
    // uint8_t g = (uint8_t)(c >> 8);
    // uint8_t b = (uint8_t)c;
    // Blue is always slightly on; R and G cycle out of sync.
    if (m_pixels_1)
    {
      for (int i = 0; i <= get_num_leds_to_update(*m_pixels_1); i++)
      {
        float x = i / 100. - 0.5 * input.t;
        uint8_t r = value * (cos(x * 1.) + 1.) / 2.;
        uint8_t g = value * (cos(x * 2) + 1.) / 2.;
        uint8_t b = value * (cos(x * 3) + 2.) / 3.;
        setPixels1Color(i, r, g, b);
      }
      m_pixels_1->show();
    }

    if (m_pixels_2)
    {
      for (int i = 0; i <= get_num_leds_to_update(*m_pixels_2); i++)
      {
        float x = i / 100. - 0.5 * input.t;
        uint8_t r = value * (cos(x * 1.) + 1.) / 2.;
        uint8_t g = value * (cos(x * 2) + 1.) / 2.;
        uint8_t b = value * (cos(x * 3) + 2.) / 3.;
        setPixels2Color(i, r, g, b);
      }
      m_pixels_2->show();
    }
  }

  void update_party_mode_rolling(ControlInput input)
  {
    // Use total RGB brightness but not colors.
    uint8_t value = sqrt(pow(input.color.r, 2) + pow(input.color.g, 2) + pow(input.color.b, 2.));
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

    if (m_pixels_1)
    {
      for (int i = 0; i <= get_num_leds_to_update(*m_pixels_1); i++)
      {
        setPixels1Color(i, r, g, b);
      }
      m_pixels_1->show();
    }

    if (m_pixels_2)
    {
      for (int i = 0; i <= get_num_leds_to_update(*m_pixels_2); i++)
      {
        setPixels2Color(i, r, g, b);
      }
      m_pixels_2->show();
    }
  }

  void update(ControlInput input)
  {
    if (input.control_mode != m_last_control_mode){
      m_last_control_mode = input.control_mode;
      m_last_mode_change_ms = millis();
    }

    if (!input.on_off)
    {
      m_last_mode_change_ms = millis();
      turn_off_all_leds();
    }
    else
    {
      // Dispatch to mode-specific controller.
      switch (input.control_mode)
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