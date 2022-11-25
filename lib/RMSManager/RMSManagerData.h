#include <Arduino.h>

enum JsonRequestType {
    vcellRequest,
    tempRequest,
    vpackRequest,
    balancingRequest,
};

struct LedColor {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};