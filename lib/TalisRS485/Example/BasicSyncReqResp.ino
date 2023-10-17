#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <TalisRS485Handler.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

const char* TAG = "Basic Sync Req Resp";

TalisRS485Handler talis;

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

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    Serial2.begin(115200);
    talis.begin(&Serial2, true, '\n');
    talis.setTimeout(500);
    lastTime = millis();
}

void loop() {
    talis.handle();
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

