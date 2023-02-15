#pragma once

class StatusLEDManager
{
public:
    unsigned long flip_time_ms = 0;

    StatusLEDManager(int led_pin) : m_led_pin(led_pin)
    {
    }

    bool setup()
    {
        pinMode(m_led_pin, OUTPUT);
        return true;
    }

    void update()
    {
        unsigned long t = millis();
        if (t - m_last_flip_time_ms > flip_time_ms)
        {
            m_last_flip_time_ms = t;
            m_led_state = !m_led_state;
            digitalWrite(LED_BUILTIN, m_led_state);
        }
    }

private:
    int m_led_pin;
    int m_led_state = 0;
    unsigned long m_last_flip_time_ms = 0;
};
