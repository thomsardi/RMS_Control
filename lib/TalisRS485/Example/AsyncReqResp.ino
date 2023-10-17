#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <TalisRS485Handler.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

const char* TAG = "Basic Sync Req Resp";

TalisRS485Handler talis;
std::vector<TalisRS485TxMessage> userCommand;

QueueHandle_t rs485ReceiverQueue = xQueueCreate(10, sizeof(TalisRS485RxMessage));
TaskHandle_t rs485ReceiverTaskHandle;
TaskHandle_t rs485TransmitterTaskHandle;

unsigned long lastTime;

void onDataCb(TalisRS485RxMessage msg, uint32_t token)
{
    ESP_LOGV(TAG, "Message token = %d\n", token);
    ESP_LOG_BUFFER_CHAR(TAG, msg.rxData, msg.dataLength);
}

void onErrorCb(TalisRS485::Error errorCode, uint32_t token)
{
    ESP_LOGV(TAG, "Message token = %d\n", token);
    ESP_LOGV(TAG, "Error code : %d\n", errorCode);
}

void rs485ReceiverTask(void *pv)
{
  const char* TAG = "RS485 Receiver Task";
  esp_log_level_set(TAG, ESP_LOG_VERBOSE);

  while (1)
  {
    TalisRS485RxMessage msgBuffer;
    if (xQueueReceive(rs485ReceiverQueue, &msgBuffer, portMAX_DELAY) == pdTRUE)
    {  
      ESP_LOGV(TAG, "Message token : %d\n", msgBuffer.token);
      if (msgBuffer.token >= 1000 && msgBuffer.token < 2000)
      {
        ESP_LOGV(TAG, "command from user");
      }
      
      if (msgBuffer.dataLength < 0)
      {
        ESP_LOGV(TAG, "Error on received");
        switch (msgBuffer.error)
        {
        case TalisRS485::Error::NO_TERMINATE_CHARACTER :
          ESP_LOGV(TAG, "No terminate character found");
          break;
        case TalisRS485::Error::TIMEOUT :
          ESP_LOGV(TAG, "Timeout");
          break;
        case TalisRS485::Error::BUFFER_OVF :
          ESP_LOGV(TAG, "Buffer Overflow");
        default:
          break;
        }
      }
      else
      {
        ESP_LOGV(TAG, "Length : %d\n", msgBuffer.dataLength);
        ESP_LOG_BUFFER_CHAR(TAG, msgBuffer.rxData, msgBuffer.dataLength);
      }
    }
  }
}


void rs485TransmitterTask(void *pv)
{
  const char* TAG = "RS485 Transmitter Task";
  esp_log_level_set(TAG, ESP_LOG_VERBOSE);

  while (1)
  {
    talis.handle();
    vTaskDelay(1);
  }
}

void setup() {
    userCommand.reserve(5);
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    Serial2.begin(115200);
    talis.begin(&Serial2, true, '\n');
    talis.setTimeout(500);

    xTaskCreate(rs485ReceiverTask, "RS485 Receiver Task", 4096, NULL, 10, &rs485ReceiverTaskHandle);
    xTaskCreate(rs485TransmitterTask, "RS485 Transmitter Task", 4096, NULL, 5, &rs485TransmitterTaskHandle);

    lastTime = millis();
}

void loop() {
    
    if (userCommand.size() > 0)
    {
        // ESP_LOGV(TAG, "User command size : %d\n", userCommand.size());
        TalisRS485::Error error = talis.addRequest(userCommand.at(0));
        if (error == TalisRS485::Error::SUCCESS)
        {
            userCommand.erase(userCommand.begin());
        }  
    }
    else
    {
        if (millis() - lastTime > 2000)
        {
            TalisRS485TxMessage txMsg;
            txMsg.token = 1000;
            txMsg.id = 1;
            txMsg.requestCode = TalisRS485::RequestType::VCELL;
            TalisRS485Message::createVcellRequest(txMsg);
            talis.addRequest(txMsg);
            lastTime = millis();
        } 
    } 
}

