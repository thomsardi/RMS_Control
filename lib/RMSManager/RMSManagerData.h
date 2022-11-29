#include <Arduino.h>

enum ReadResponseType {
    error,
    vcellResponse,
    vpackResponse,
    tempResponse
};

struct LedColor {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};