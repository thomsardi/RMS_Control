#include <Arduino.h>

struct LedData
{
    int currentGroup = -1;
    int currentString = -1;
    int red[8] = {0};
    int green[8] = {0};
    int blue[8] = {0};
};