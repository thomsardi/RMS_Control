#include <Arduino.h>

enum ReadResponseType {
    noResponse = 0,
    vcellResponse = 1,
    tempResponse = 2,
    vpackResponse = 4,
    other = 0
};

struct LedColor {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};