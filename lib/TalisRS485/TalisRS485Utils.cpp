#include "TalisRS485Utils.h"

TalisRS485Utils::TalisRS485Utils(/* args */)
{
    
}

uint32_t TalisRS485Utils::calculateInterval(uint32_t baudRate)
{
    // calculateInterval: determine the minimal gap time between messages
  uint32_t interval = 0;

  // silent interval is at least 3.5x character time
  interval = 35000000UL / baudRate;  // 3.5 * 10 bits * 1000 µs * 1000 ms / baud
  if (interval < 1750) interval = 1750;       // lower limit according to Modbus RTU standard
  LOG_V("Calc interval(%u)=%u\n", baudRate, interval);
  return interval;
}