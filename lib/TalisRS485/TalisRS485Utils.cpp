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

void TalisRS485Utils::socToLed(uint8_t packNumber, uint8_t ledPerPack, int soc)
{
  /**
   * Case for 60% SoC in 4 packs with 8 leds per pack
   * 
   * Total led = 8 * 4 = 32 leds
   * % for 1 led = 100 / 32 = 3.125%
   * fully on led = 60 % / 3.125 % = 19.2 leds
   * fully on pack led = 19.2 / 8 = 2.4 (1st & 2nd pack fully on, 3rd pack only 20% led on)
  */
  const char* TAG = "SoC to Led";
  esp_log_level_set(TAG, ESP_LOG_INFO);
  uint16_t totalLeds = packNumber*ledPerPack; // 16 leds
  ESP_LOGI(TAG, "Total leds : %d\n", totalLeds);
  float percentagePerLed = 100 / totalLeds; // 6.25%
  ESP_LOGI(TAG, "Percentage per leds : %f\n", percentagePerLed);
  uint8_t numberOfOnLed = soc / percentagePerLed; // 9.6 leds
  float division = numberOfOnLed / ledPerPack; // 9.6 / 8 = 1.2
  ESP_LOGI(TAG, "Division : %f\n", division);
  uint8_t fullyOnPackLed = floorf(division); // 9.6 / 8 = 1
  uint8_t fullyOffPackLed = packNumber - fullyOnPackLed;
  ESP_LOGI(TAG, "Fully on pack : %d\n", fullyOnPackLed);
  ESP_LOGI(TAG, "Fully off pack : %d\n", fullyOffPackLed);
  uint8_t halfOnOffPackLed = packNumber - (fullyOnPackLed + fullyOffPackLed);
  uint8_t offLed = (division - fullyOnPackLed) * ledPerPack; // (1.2 - 1) * 8 = 0.2 * 8 = 1.6 ~= 1
  uint8_t onLed = ledPerPack - offLed; // 8 - 1 = 7
  // ESP_LOGI("TAG", "Total Fully on pack : %d\n", fullyOnPackLed);
  // for (size_t i = 0; i < fullyOnPackLed; i++)
  // {
  //   ESP_LOGI(TAG, "Fully On Pack Led : %d\n", i);
  // }
  
  // if (halfOnOffPackLed)
  // {
  //   ESP_LOGI(TAG, "Pack number half on-off : %d\n", fullyOnPackLed+1);
  //   for (size_t i = 0; i < offLed; i++)
  //   {
  //     ESP_LOGI(TAG, "Off led %d\n", i);
  //   }
  //   for (size_t i = 0; i < onLed; i++)
  //   {
  //     ESP_LOGI(TAG, "On led %d\n", i);
  //   }
  // }  
  // for (size_t i = 0; i < fullyOffPackLed; i++)
  // {
  //   ESP_LOGI(TAG, "Fully Off Pack Led : %d\n", fullyOnPackLed+2+i);
  // }
  
}