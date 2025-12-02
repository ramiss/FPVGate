#include "status_led.h"
#include "config/config.h"

// Helper function to set LED state (GPIO or WS2812)
void StatusLed::_setLedState(uint8_t state) {
#if defined(STATUS_LED_WS2812) && STATUS_LED_WS2812
    // WS2812 mode: use rgbLedWrite() (replaces deprecated neopixelWrite)
    // HIGH = on (use stored color), LOW = off (0,0,0)
    if (state == HIGH) {
        rgbLedWrite(_ledPin, _ws2812_r, _ws2812_g, _ws2812_b);
    } else {
        rgbLedWrite(_ledPin, 0, 0, 0);  // Off/black
    }
#else
    // GPIO mode: use digitalWrite
    digitalWrite(_ledPin, state);
#endif
}

void StatusLed::init(uint8_t pin, bool inverted) {
    
    _ledPin = pin;
    _inverted = inverted;
    _initialState = inverted ? HIGH : LOW;
    _currentState = _initialState;
    
#if defined(STATUS_LED_WS2812) && STATUS_LED_WS2812
    // WS2812 mode: Initialize with rgbLedWrite() (replaces deprecated neopixelWrite)
    // Pin is 10 for ESP32-C3-Zero, just call rgbLedWrite(pin, r, g, b)
    // Do NOT call pinMode() - rgbLedWrite() handles everything
    rgbLedWrite(_ledPin, 0, 0, 0);  // Start off (black)
#else
    // GPIO mode: standard LED
    pinMode(_ledPin, OUTPUT);
    digitalWrite(_ledPin, _currentState);
#endif
    
    _state = STATUS_LED_IDLE;
}

void StatusLed::update(uint32_t currentTimeMs) {
    // Early return for idle state - no work needed
    if (_state == STATUS_LED_IDLE) {
        return;
    }
    
    // Time wrap-around protection (check once at start)
    if (currentTimeMs < _lastUpdateMs) {
        _lastUpdateMs = currentTimeMs;
        return;
    }
    
    switch (_state) {
        case STATUS_LED_BLINKING:
            // Continuous blink pattern - only update when state needs to change
            if (_currentState == !_initialState) {
                // Currently ON - check if time to turn OFF
                if ((currentTimeMs - _lastUpdateMs) >= _onTimeMs) {
                    _currentState = _initialState;
                    _setLedState(_currentState);
                    _lastUpdateMs = currentTimeMs;
                }
            } else {
                // Currently OFF - check if time to turn ON
                if ((currentTimeMs - _lastUpdateMs) >= _offTimeMs) {
                    _currentState = !_initialState;
                    _setLedState(_currentState);
                    _lastUpdateMs = currentTimeMs;
                }
            }
            break;
            
        case STATUS_LED_ON:
            // LED on for specific duration
            if (_onTimeMs > 0 && (currentTimeMs - _lastUpdateMs) >= _onTimeMs) {
                // Time to turn off
                _currentState = _initialState;
                _setLedState(_currentState);
                _state = STATUS_LED_IDLE;
            }
            break;
            
        case STATUS_LED_ACTIVITY:
            // Quick flash on activity - automatically returns to idle
            if ((currentTimeMs - _lastUpdateMs) >= ACTIVITY_FLASH_MS) {
                // Flash duration elapsed - turn off
                _currentState = _initialState;
                _setLedState(_currentState);
                _state = STATUS_LED_IDLE;
            }
            break;
            
        default:
            break;
    }
}

void StatusLed::on(uint32_t timeMs) {
    
    if (timeMs > 0) {
        _state = STATUS_LED_ON;
        _onTimeMs = timeMs;
    } else {
        _state = STATUS_LED_IDLE;
    }
    
    _currentState = !_initialState;
    _setLedState(_currentState);
    _lastUpdateMs = millis();
}

void StatusLed::off() {
    
    _state = STATUS_LED_IDLE;
    _currentState = _initialState;
    _setLedState(_currentState);
}

void StatusLed::blink(uint32_t onTimeMs, uint32_t offTimeMs) {
    
    _onTimeMs = onTimeMs;
    if (offTimeMs > 0) {
        _offTimeMs = offTimeMs;
    } else {
        _offTimeMs = onTimeMs;  // Default: equal on/off time
    }
    
    _state = STATUS_LED_BLINKING;
    _currentState = !_initialState;  // Start with LED ON
    _setLedState(_currentState);
    _lastUpdateMs = millis();
}

void StatusLed::activity() {
    
    _state = STATUS_LED_ACTIVITY;
    _currentState = !_initialState;  // Turn LED ON
    _setLedState(_currentState);
    _lastUpdateMs = millis();
}

void StatusLed::setColor(uint8_t r, uint8_t g, uint8_t b) {
#if defined(STATUS_LED_WS2812) && STATUS_LED_WS2812
    
    // Store color and update immediately (like Waveshare example)
    _ws2812_r = r;
    _ws2812_g = g;
    _ws2812_b = b;
    
    // Update LED if it's currently on
    if (_currentState == !_initialState) {
        rgbLedWrite(_ledPin, _ws2812_r, _ws2812_g, _ws2812_b);
    }
#else
    // GPIO LED - color setting not supported, just ignore
    (void)r; (void)g; (void)b;
#endif
}
