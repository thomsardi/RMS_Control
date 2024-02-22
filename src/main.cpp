/*
  ShiftRegister74HC595 - Library for simplified control of 74HC595 shift registers.
  Developed and maintained by Timo Denk and contributers, since Nov 2014.
  Additional information is available at https://timodenk.com/blog/shift-register-arduino-library/
  Released into the public domain.
*/

#include <ShiftRegister74HC595.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Vector.h>
#include <ArduinoOTA.h>
#include <ESP32httpUpdate.h>
// #include <AsyncElegantOTA.h>
#include <HTTPClient.h>
#include <JsonManager.h>
#include <AsyncJson.h>
#include <RMSManager.h>
#include <Updater.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ESPmDNS.h>
#include <LedAnimation.h>
#include <EEPROM.h>
#include <OneButton.h>
#include <ModbusServerTCPasync.h>
#include <ModbusRegisterHandler.h>
#include <Preferences.h>
#include <nvs_flash.h>
#include <Utilities.h>
#include <TalisRS485Handler.h>
#include <WiFiSetting.h>
#include <TalisMemory.h>
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

// #define FZ_NOHTTPCLIENT
#define FZ_WITH_ASYNCSRV
#include <flashz-http.hpp>
#include <flashz.hpp>

#include "defs.h"
#include <map>
#include "LittleFS.h"

#define USE_BQ76940 1

#define VERSION "1.0.0"


// #define LAMINATE_ROOM 1 //uncomment to use board in laminate room

#define CYCLING 1

#define HARDWARE_ALARM 1

#define DEBUG 1

// #define DEBUG_STATIC 1

#ifdef LAMINATE_ROOM
    #define SERIAL_DATA 14
    #define SHCP 13
    #define STCP 12
    #define HOST_NAME "RMS-Laminate-Room"
#else
    #define SERIAL_DATA 14
    #define SHCP 13
    #define STCP 12
    #define HOST_NAME "RMS-Rnd-Room"
#endif

const char* TAG = "RMS-Control-Event";

std::vector<TalisRS485TxMessage> userCommand;
std::vector<int> addressList;

QueueHandle_t rs485ReceiverQueue = xQueueCreate(10, sizeof(TalisRS485RxMessage));
TaskHandle_t rs485ReceiverTaskHandle;
TaskHandle_t rs485TransmitterTaskHandle;
TaskHandle_t otaTaskHandle;

int battRelay = 23;
int buzzer = 26;

int internalLed = 2;

int const numOfShiftRegister = 8;

ShiftRegister74HC595<numOfShiftRegister> sr(SERIAL_DATA, SHCP, STCP);

hw_timer_t *myTimer = NULL;

#ifdef DEBUG
    // const char *ssid = "Redmi";
    // const char *password = "thomasredmi15";
    const char *ssid = "RnD_Sundaya";
    // const char *ssid = "Zali cerita cerita";
    const char *password = "sundaya22";
    // const char *ssid = "Sundaya";
    // const char *password = "sundaya21";
#else
    // const char *ssid = "RnD_Sundaya";
    const char *ssid = "Ruang_Laminate";
    const char *password = "sundaya22";
#endif


// Set your Gateway IP address
// IPAddress primaryDNS(192, 168, 2, 1);        // optional
// IPAddress secondaryDNS(119, 18, 156, 10);       // optional

AsyncWebServer server(80);
TalisRS485Handler talis;
WiFiSetting wifiSetting;
TalisMemory talisMemory;
JsonManager jsonManager;
RMSManager rmsManager;
LedAnimation ledAnimation(8,8, true);
std::map<int, Updater> updater;
OneButton startButton(32, true, true);

ModbusServerTCPasync MBserver; 

PackedData packedData;
std::map<int, CMSData> cmsData;
std::map<int, CellBalanceState> cellBalanceState;
AlarmParam alarmParam;
CellBalancingCommand cellBalancingCommand;
AlarmCommand alarmCommand;
DataCollectionCommand dataCollectionCommand;
DataCollectionCommand testDataCollection;
CommandStatus commandStatus;
CMSRestartCommand cmsRestartCommand;
RMSRestartCommand rmsRestartCommand;

SettingRegisters settingRegisters;
OtherInfo otherInfo;
SystemStatus systemStatus;
MbusCoilData mbusCoilData;
ModbusRegisterData modbusRegisterData;
uint16_t saveParam;
uint16_t ssidArr[8];
uint16_t passArr[8];
uint16_t saveNetwork;
uint16_t ipOctet[4] = {192, 168, 2, 141};
uint16_t gatewayOctet[4] = {192, 168, 2, 1};
uint16_t subnetOctet[4] = {255, 255, 255, 0};
uint16_t gServerType = 2;
uint16_t gMode = 2;

uint8_t id = 1;
uint8_t command;

bool isAddressing = false;
bool beginAddressing = false;
bool isAddressed = false;
int addressOut = 23;

enum CommandType {
    VCELL = 0,
    TEMP = 1,
    VPACK = 2,
    CMSSTATUS = 3,
    CMSREADBALANCINGSTATUS = 4,
    LED = 5,
    CMSINFO = 6,
    CMSFRAMEWRITE = 7,
    CMSCODEWRITE = 8,
    CMSBASECODEWRITE = 9,
    CMSMCUCODEWRITE = 10,
    CMSSITELOCATIONWRITE = 11,
    BALANCINGWRITE = 12,
    SHUTDOWN = 13,
    WAKEUP = 14,
    RESTART = 15
};

// uint16_t msgCount[16];
// int addressListStorage[32];
// Vector<int> addressList(addressListStorage);

uint8_t currentAddrBid;

bool isCmsRestartPin = false;
bool addressingByButton = false;
bool hardwareAlarmEnable = false;
bool manualOverride = false;
bool factoryReset = false;
bool forceBuzzer = false;
bool forceRelay = false;

unsigned long lastReconnectMillis = 0;
unsigned long lastSent = 0;
unsigned long lastErrorCounterCheck = 0;
unsigned long lastCheckQueue = 0;
int reconnectInterval = 5000;
String rmsCode = "RMS-32-NA";
String rackSn = "RACK-32-NA";
String mac = WiFi.macAddress();

uint8_t commandOrder[7] = {
    TalisRS485::RequestType::CMSINFO,
    TalisRS485::RequestType::VCELL,
    TalisRS485::RequestType::TEMP,
    TalisRS485::RequestType::VPACK,
    TalisRS485::RequestType::CMSSTATUS,
    TalisRS485::RequestType::CMSREADBALANCINGSTATUS,
    TalisRS485::RequestType::LED
};

void declareStruct()
{
    packedData.rackSn = &rackSn;
    packedData.mac = &mac;
    packedData.cms = &cmsData;
}

int readLed(const String &input)
{
    int bid = 0;
    int startIndex = 0;
    int status = -1;
    bool isAllDataCaptured = false;
    bool isAllDataNormal = false;
    bool isValidJsonFormat = true;
    DynamicJsonDocument docBattery(1024);
    DeserializationError error = deserializeJson(docBattery, input);
    // Serial.println("Read Led Response");
    if (error) 
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return status;
    }
    // deserializeJson(docBattery, Serial2);
    JsonObject object = docBattery.as<JsonObject>();
    // Serial.println(jsonDoc);
    if (!object.isNull())
    {
        if (docBattery.containsKey("LEDSET"))
        {
            // status = docBattery["STATUS"];
            // Serial.println("Contain LEDSET");
            status = docBattery["LEDSET"];
        }
        else
        {
            return status;
        }
    }
    return status;
}

int readFrameWriteResponse(const String &input)
{
    int bid = 0;
    int startIndex = 0;
    int status = -1;
    bool isAllDataCaptured = false;
    bool isAllDataNormal = false;
    bool isValidJsonFormat = true;
    DynamicJsonDocument docBattery(1024);
    DeserializationError error = deserializeJson(docBattery, input);
    // Serial.println("Read Frame Write Response");
    if (error) 
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return status;
    }
    // deserializeJson(docBattery, Serial2);
    JsonObject object = docBattery.as<JsonObject>();
    // Serial.println(jsonDoc);
    if (!object.isNull())
    {
        if (docBattery.containsKey("frame_write"))
        {
            status = docBattery["status"];
            // status = 1;
        }
        else
        {
            return status;
        }
    }
    return status;
}

int readCMSCodeWriteResponse(const String &input)
{
    int bid = 0;
    int startIndex = 0;
    int status = -1;
    bool isAllDataCaptured = false;
    bool isAllDataNormal = false;
    bool isValidJsonFormat = true;
    DynamicJsonDocument docBattery(1024);
    DeserializationError error = deserializeJson(docBattery, input);
    // Serial.println("Read Frame Write Response");
    if (error) 
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return status;
    }
    // deserializeJson(docBattery, Serial2);
    JsonObject object = docBattery.as<JsonObject>();
    // Serial.println(jsonDoc);
    if (!object.isNull())
    {
        if (docBattery.containsKey("cms_write"))
        {
            status = docBattery["status"];
            // status = 1;
        }
        else
        {
            return status;
        }
    }
    return status;
}

int readCMSBaseCodeWriteResponse(const String &input)
{
    int bid = 0;
    int startIndex = 0;
    int status = -1;
    bool isAllDataCaptured = false;
    bool isAllDataNormal = false;
    bool isValidJsonFormat = true;
    DynamicJsonDocument docBattery(1024);
    DeserializationError error = deserializeJson(docBattery, input);
    // Serial.println("Read Frame Write Response");
    if (error) 
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return status;
    }
    // deserializeJson(docBattery, Serial2);
    JsonObject object = docBattery.as<JsonObject>();
    // Serial.println(jsonDoc);
    if (!object.isNull())
    {
        if (docBattery.containsKey("base_write"))
        {
            status = docBattery["status"];
            // status = 1;
        }
        else
        {
            return status;
        }
    }
    return status;
}

int readCMSMcuCodeWriteResponse(const String &input)
{
    int bid = 0;
    int startIndex = 0;
    int status = -1;
    bool isAllDataCaptured = false;
    bool isAllDataNormal = false;
    bool isValidJsonFormat = true;
    DynamicJsonDocument docBattery(1024);
    DeserializationError error = deserializeJson(docBattery, input);
    // Serial.println("Read Frame Write Response");
    if (error) 
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return status;
    }
    // deserializeJson(docBattery, Serial2);
    JsonObject object = docBattery.as<JsonObject>();
    // Serial.println(jsonDoc);
    if (!object.isNull())
    {
        if (docBattery.containsKey("mcu_write"))
        {
            status = docBattery["status"];
            // status = 1;
        }
        else
        {
            return status;
        }
    }
    return status;
}

int readCMSSiteLocationWriteResponse(const String &input)
{
    int bid = 0;
    int startIndex = 0;
    int status = -1;
    bool isAllDataCaptured = false;
    bool isAllDataNormal = false;
    bool isValidJsonFormat = true;
    DynamicJsonDocument docBattery(1024);
    DeserializationError error = deserializeJson(docBattery, input);
    // Serial.println("Read Frame Write Response");
    if (error) 
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return status;
    }
    // deserializeJson(docBattery, Serial2);
    JsonObject object = docBattery.as<JsonObject>();
    // Serial.println(jsonDoc);
    if (!object.isNull())
    {
        if (docBattery.containsKey("site_write"))
        {
            status = docBattery["status"];
            // status = 1;
        }
        else
        {
            return status;
        }
    }
    return status;
}

void convertLedDataToLedCommand(const LedData &ledData, LedCommand &ledCommand)
{
    ledCommand.bid = addressList.at(ledData.currentGroup);
    ledCommand.ledset = 1;
    ledCommand.num_of_led = 8;
    for (size_t i = 0; i < ledCommand.num_of_led; i++)
    {
        ledCommand.red[i] = ledData.red[i];
        ledCommand.green[i] = ledData.green[i];
        ledCommand.blue[i] = ledData.blue[i];
    }
}

void addressing(bool isFromBottom, size_t numOfPacks)
{
    for (size_t i = 0; i < numOfPacks; i++)
    {
        ESP_LOGI(TAG, "Increment : %d\n", i);
        uint8_t x;
        if(isFromBottom)
        {
            x = (8 * (numOfPacks - i)) - 1; // Q7 is connected to addressing pin of CMS
        }
        else
        {
            x = (numOfPacks * i) + 7;
        }
        int bid = i + 1; // id start from 1
        sr.set(x, HIGH);
        delay(100);
        sr.set(x, LOW);
        delay(400);
        digitalWrite(addressOut, LOW);
        delay(100);
        digitalWrite(addressOut, HIGH);
        while(talis.handleAddressing());
    } 
}

// Server function to handle FC 0x01
ModbusMessage FC01(ModbusMessage request) {
    ModbusMessage response;      // The Modbus message we are going to give back

    ModbusRegisterHandler mrh(modbusRegisterData);
    return mrh.handleReadCoils(request);
    // Send response back
    // return response;
}

// Server function to handle FC 0x03 and 0x04
ModbusMessage FC03(ModbusMessage request) {
    ModbusMessage response;      // The Modbus message we are going to give back
    uint16_t addr = 0;           // Start address
    uint16_t words = 0;          // # of words requested
    request.get(2, addr);        // read address from request
    request.get(4, words);       // read # of words from request

    ModbusRegisterHandler mrh(modbusRegisterData);
    return mrh.handleReadHoldingRegisters(request);
    // Send response back
    // return response;
}

ModbusMessage FC04(ModbusMessage request) {
    ModbusMessage response;      // The Modbus message we are going to give back
    uint16_t addr = 0;           // Start address
    uint16_t words = 0;          // # of words requested
    request.get(2, addr);        // read address from request
    request.get(4, words);       // read # of words from request

    otherInfo.data[0] = addressList.size();
    otherInfo.data[1] = systemStatus.val;
    
    // for (size_t i = 0; i < 8; i++)
    // {
    //     cellData[i].bid = i + 1;
    //     Utilities::fillArrayRandom<int>(cellData[i].vcell, 45, 2800, 3800);
    //     Utilities::fillArrayRandom<int32_t>(cellData[i].temp, 9, 10000, 100000);
    //     Utilities::fillArrayRandom<int32_t>(cellData[i].pack, 3, 32000, 40000);
    // }
    
    ModbusRegisterHandler mrh(modbusRegisterData);
    return mrh.handleReadInputRegisters(request);
    
    // Send response back
    // return response;
}

ModbusMessage FC05(ModbusMessage request) {
    ModbusMessage response;      // The Modbus message we are going to give back
    
    ModbusRegisterHandler mrh(modbusRegisterData);
    return mrh.handleWriteCoil(request);
    
    // Send response back
    // return response;
}

ModbusMessage FC06(ModbusMessage request) {
    ModbusMessage response;      // The Modbus message we are going to give back
    uint16_t addr = 0;           // Start address
    uint16_t words = 0;          // # of words requested
    request.get(2, addr);        // read address from request
    request.get(4, words);       // read # of words from request
    
    ModbusRegisterHandler mrh(modbusRegisterData);
    return mrh.handleWriteRegister(request);
    
    // Send response back
    // return response;
}

ModbusMessage FC16(ModbusMessage request) {
    ModbusMessage response;      // The Modbus message we are going to give back
    uint16_t addr = 0;           // Start address
    uint16_t words = 0;          // # of words requested
    request.get(2, addr);        // read address from request
    request.get(4, words);       // read # of words from request
    
    ModbusRegisterHandler mrh(modbusRegisterData);
    return mrh.handleWriteMultipleRegisters(request);
    
    // Send response back
    // return response;
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
    ESP_LOGI(TAG, "Connected to %s\n", WiFi.SSID().c_str());
    ESP_LOGI(TAG, "IP Address : %s\n", WiFi.localIP().toString().c_str());
    ESP_LOGI(TAG, "Subnet : %s\n", WiFi.subnetMask().toString().c_str());
    ESP_LOGI(TAG, "Gateway : %s\n", WiFi.gatewayIP().toString().c_str());
    ESP_LOGI(TAG, "DNS 1 : %s\n", WiFi.dnsIP(0).toString().c_str());
    ESP_LOGI(TAG, "DNS 2 : %s\n", WiFi.dnsIP(1).toString().c_str());
    ESP_LOGI(TAG, "Hostname : %s\n", WiFi.getHostname());
    digitalWrite(internalLed, HIGH);
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
    ESP_LOGI(TAG, "Wifi Connected");
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
    digitalWrite(internalLed, LOW);
    ESP_LOGI(TAG, "Disconnected from WiFi access point\n");
    ESP_LOGI(TAG, "WiFi lost connection. Reason: %d\n", info.wifi_sta_disconnected.reason);
    ESP_LOGI(TAG, "Trying to Reconnect\n");
    WiFi.disconnect();
    WiFi.reconnect();
}

void buttonClicked()
{
    // Serial.println("Clicked");
    addressingByButton = 1;
}

void buttonLongPressed()
{
    dataCollectionCommand.exec = 0;
    alarmCommand.buzzer = 0;
    cmsRestartCommand.restart = 1;
    timerStop(myTimer);
    timerWrite(myTimer, 0);
}

void buttonDoubleClicked()
{
    dataCollectionCommand.exec = 0;
    alarmCommand.buzzer = 0;
    cmsRestartCommand.restart = 1;
    timerStop(myTimer);
    timerWrite(myTimer, 0);
}

void IRAM_ATTR onTimer()
{
    if (!manualOverride)
    {
        if (alarmCommand.buzzer)
        {
            digitalWrite(buzzer, !digitalRead(buzzer));
        }
        else
        {
            digitalWrite(buzzer, LOW);
        }
    }
    else
    {
        if (forceBuzzer)
        {
            digitalWrite(buzzer, !digitalRead(buzzer));
        }
        else
        {
            digitalWrite(buzzer, LOW);
        }
    }
           
}

void onDataCb(TalisRS485RxMessage msg, uint32_t token)
{
  ESP_LOGI(TAG, "data on id : %d\n", msg.id);
  xQueueSend(rs485ReceiverQueue, &msg, 0);
}

// void onErrorCb(TalisRS485::Error errorCode, uint32_t token)
// {
//   TalisRS485RxMessage msg;
//   msg.token = token;
//   msg.error = errorCode;
//   xQueueSend(rs485ReceiverQueue, &msg, 0);
// }

void onErrorCb(TalisRS485ErrorMessage errMsg, uint32_t token)
{
  TalisRS485RxMessage msg;
  msg.id = errMsg.id;
  msg.token = token;
  msg.error = errMsg.errorCode;
  ESP_LOGI(TAG, "error on id : %d\n", msg.id);
  xQueueSend(rs485ReceiverQueue, &msg, 0);
}

void addressingHandler(TalisRS485Handler &talis, const TalisRS485RxMessage &data, uint8_t &currentAddrBid)
{
  const char* TAG = "Addressing handler";
  String s;
  for (size_t i = 0; i < data.dataLength; i++)
  {
    s += (char)data.rxData[i];
  }
  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, s);
  
  if (doc.containsKey("BID_STATUS"))
  {
    ESP_LOGI(TAG, "found \"BID_STATUS\"\n");
    TalisRS485TxMessage txMsg;
    txMsg.token = 2000;
    String output;
    StaticJsonDocument<16> doc;
    doc["BID_ADDRESS"] = currentAddrBid;
    serializeJson(doc, output);
    output += '\n';
    txMsg.id = currentAddrBid;
    txMsg.dataLength = output.length();
    txMsg.requestCode = TalisRS485::RequestType::ADDRESS;
    txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());
    Serial.print(output);
    // ESP_LOGV(TAG, "Serialize json : \n%s\n", output);
    talis.send(txMsg);
  }

  if (doc.containsKey("RESPONSE"))
  {
    ESP_LOGI(TAG, "Addr bid : %d", currentAddrBid);
    ESP_LOGI(TAG, "Increment bid");
    addressList.push_back(currentAddrBid);
    currentAddrBid++;
  }
}

void rs485ReceiverTask(void *pv)
{
    const char* TAG = "RS485 Receiver Task";
    esp_log_level_set(TAG, ESP_LOG_INFO);

    while (1)
    {
        TalisRS485RxMessage msgBuffer;
        if (xQueueReceive(rs485ReceiverQueue, &msgBuffer, portMAX_DELAY) == pdTRUE)
        {  
            // ESP_LOGI(TAG, "Serial incoming");
            // ESP_LOGI(TAG, "Message token : %d\n", msgBuffer.token);
            if (msgBuffer.token >= 1000 && msgBuffer.token < 2000)
            {
                ESP_LOGI(TAG, "command from user");
            }
            
            if (msgBuffer.dataLength < 0)
            {
                
                switch (msgBuffer.error)
                {
                    case TalisRS485::Error::NO_TERMINATE_CHARACTER :
                        ESP_LOGI(TAG, "No terminate character found");
                        talis.updateOnError(msgBuffer.id, cmsData);
                    break;
                    case TalisRS485::Error::TIMEOUT :
                        ESP_LOGI(TAG, "Error on received id : %d\n", msgBuffer.id);
                        ESP_LOGI(TAG, "Timeout");
                        talis.updateOnError(msgBuffer.id, cmsData);
                    break;
                    case TalisRS485::Error::BUFFER_OVF :
                        ESP_LOGI(TAG, "Buffer Overflow");
                        talis.updateOnError(msgBuffer.id, cmsData);
                    default:
                    break;
                }
            }
            else
            {
                size_t arrSize = msgBuffer.rxData.size();
                char c[arrSize];
                memcpy(c, msgBuffer.rxData.data(), arrSize);
                int bid;
                ESP_LOGI(TAG, "processed data at id : %d\n", msgBuffer.id);
                switch (msgBuffer.token)
                {
                case TalisRS485::RequestType::CMSINFO :
                    // ESP_LOGI(TAG, "reading cms info");
                    bid = talis.readInfo(String(c), cmsData);
                    break;
                case TalisRS485::RequestType::VCELL :
                    // ESP_LOGI(TAG, "reading vcell");
                    bid = talis.readCell(String(c), cmsData);
                    if (bid > 0)
                    {
                        updater[bid].updateVcell(talis.checkCell(cmsData[bid], alarmParam));
                    }
                    break;
                case TalisRS485::RequestType::TEMP :
                    // ESP_LOGI(TAG, "reading temperature");
                    bid = talis.readTemperature(String(c), cmsData);
                    if (bid > 0)
                    {
                        updater[bid].updateTemp(talis.checkTemperature(cmsData[bid], alarmParam));
                    }
                    break;
                case TalisRS485::RequestType::VPACK :
                    // ESP_LOGI(TAG, "reading vpack");
                    bid = talis.readVpack(String(c), cmsData);
                    if (bid > 0)
                    {
                        updater[bid].updateVpack(talis.checkVpack(cmsData[bid]));
                    }
                    break;
                case TalisRS485::RequestType::CMSSTATUS :
                    // ESP_LOGI(TAG, "reading status");
                    bid = talis.readStatus(String(c), cmsData);
                    if (bid > 0)
                    {
                        updater[bid].updateStatus();
                        if (updater[bid].isUpdate())
                        {
                            cmsData[bid].msgCount++;
                            updater[bid].resetUpdater();
                        }
                    }
                    break;
                case TalisRS485::RequestType::CMSREADBALANCINGSTATUS :
                    // ESP_LOGI(TAG, "reading balancing status");
                    bid = talis.readBalancing(String(c), cellBalanceState, cmsData);
                    break;
                case TalisRS485::RequestType::LED :
                    bid = talis.readLed(String(c), cmsData);
                    if (bid > 0)
                    {
                        if (ledAnimation.isRunning())
                        {
                            ledAnimation.update();
                        }
                    }
                default:
                    addressingHandler(talis, msgBuffer, currentAddrBid);
                    break;
                }
                
                // ESP_LOGI(TAG, "Length : %d\n", msgBuffer.dataLength);
                // ESP_LOG_BUFFER_CHAR(TAG, msgBuffer.rxData.data(), msgBuffer.dataLength);
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
    // ESP_LOGV(TAG, "Transmitter Task");
    talis.handle();
    vTaskDelay(1);
  }
}

void otaTask(void *pv)
{
  const char* TAG = "OTA Upload Task";
  esp_log_level_set(TAG, ESP_LOG_VERBOSE);

  while (1)
  {
    // ESP_LOGV(TAG, "Transmitter Task");
    ArduinoOTA.handle();
    vTaskDelay(1);
  }
}


/**
 * Handle firmware upload
 * 
 * @brief it is used as an OTA firmware upload, pass the http received file into Update class which handle for flash writing
*/
void handleFirmwareUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    Serial.println("Handle firmware upload");
    if(!index)
    {
        Serial.printf("Update Start: %s\n", filename.c_str());
        if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
        {
            Update.printError(Serial);
        }
    }
    if(!Update.hasError())
    {
        if(Update.write(data, len) != len)
        {
            Update.printError(Serial);
        }
    }
    if(final)
    {
        if(Update.end(true))
        {
            Serial.printf("Update Success: %uB\n", index+len);
        } 
        else 
        {
            Update.printError(Serial);
        }
    }
}

/**
 * Handle firmware upload
 * 
 * @brief it is used as an OTA firmware upload, pass the http received file into Update class which handle for flash writing
*/
void handleCompressedFirmwareUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    Serial.println("Handle Compressed firmware upload");
    FlashZ &fz = FlashZ::getInstance();
    if(!index)
    {
        Serial.printf("Update Start: %s\n", filename.c_str());
        if(!fz.beginz(UPDATE_SIZE_UNKNOWN, U_FLASH))
        {
            fz.printError(Serial);
        }
    }   
    if(!fz.hasError())
    {
        Serial.println("Write to flashz");
        if(fz.writez(data, len, false) != len)
        {
            fz.printError(Serial);
        }
    }
    if(final)
    {
        if(fz.endz(true))
        {
            Serial.printf("Update Success: %uB\n", index+len);
        } 
        else 
        {
            fz.printError(Serial);
        }
    }
}

void setupLittleFs()
{
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
}

void setup()
{
    // nvs_flash_erase(); // erase the NVS partition and...
    // nvs_flash_init(); // initialize the NVS partition.
    // while(true);
    setupLittleFs();
    addressList.reserve(12);
    userCommand.reserve(5);
    xTaskCreate(rs485ReceiverTask, "RS485 Receiver Task", 4096, NULL, 10, &rs485ReceiverTaskHandle);
    xTaskCreate(rs485TransmitterTask, "RS485 Transmitter Task", 4096, NULL, 5, &rs485TransmitterTaskHandle);
    xTaskCreate(otaTask, "OTA Upload Task", 4096, NULL, 5, &otaTaskHandle);
    Serial.begin(115200); 
    Serial.setDebugOutput(true);
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "Test ESP_LOGI");

    TalisDefinition::Params params;
    String macString = WiFi.macAddress();
    macString.replace(":", "");
    params.ssid = "ESP32-" + macString;
    params.pass = "esp32-default";
    params.mode = mode_type::AP;
    params.server = server_type::STATIC;
    talisMemory.begin("talis_param", params);
    // talisMemory.reset();
    // talisMemory.setSsid("RnD_Sundaya");
    // talisMemory.setPass("sundaya22");
    // talisMemory.setMode(2);
    // talisMemory.setServer(1);
    // talisMemory.setIp("192.168.2.118");
    // talisMemory.setGateway("192.168.2.1");
    // talisMemory.setSubnet("255.255.255.0");
    talisMemory.print();
    rmsCode = talisMemory.getRmsName();
    rackSn = talisMemory.getRackSn();
    // while(1);

    alarmParam.vcell_diff = talisMemory.getCellDifference();
    alarmParam.vcell_diff_reconnect = talisMemory.getCellDifferenceReconnect();
    alarmParam.vcell_overvoltage = talisMemory.getCellOvervoltage();
    alarmParam.vcell_undervoltage = talisMemory.getCellUndervoltage();
    alarmParam.vcell_reconnect = talisMemory.getCellUndervoltageReconnect();
    alarmParam.temp_max = talisMemory.getCellOvertemperature();
    alarmParam.temp_min = talisMemory.getCellUndertemperature();

    settingRegisters.link(&alarmParam.vcell_diff, 0);
    settingRegisters.link(&alarmParam.vcell_diff_reconnect, 1);
    settingRegisters.link(&alarmParam.vcell_overvoltage, 2);
    settingRegisters.link(&alarmParam.vcell_undervoltage, 3);
    settingRegisters.link(&alarmParam.vcell_reconnect, 4);
    uint16_t *ptr = reinterpret_cast<uint16_t*>(&alarmParam.temp_max);
    settingRegisters.link(ptr+1, 5); // MSB
    settingRegisters.link(ptr, 6); // LSB
    ptr = reinterpret_cast<uint16_t*>(&alarmParam.temp_min);
    settingRegisters.link(ptr+1, 7); // MSB
    settingRegisters.link(ptr, 8); // LSB
    settingRegisters.link(&saveParam, 9);
    for (size_t i = 0; i < 8; i++)
    {
        uint16_t *ptr = ssidArr;
        settingRegisters.link(ptr+i, (10 + i));
        ptr = passArr;
        settingRegisters.link(ptr+i, (18 + i));   
    }
    for (size_t i = 0; i < 4; i++)
    {
        settingRegisters.link(&ipOctet[i], 26 + i);
        settingRegisters.link(&gatewayOctet[i], 30 + i);
        settingRegisters.link(&subnetOctet[i], 34 + i);
    }
    settingRegisters.link(&gServerType, 38);
    settingRegisters.link(&gMode, 39);
    settingRegisters.link(&saveNetwork, 40);    
    
    bool *boolPtr = reinterpret_cast<bool*>(&isAddressing);
    mbusCoilData.link(boolPtr, 0);
    boolPtr = reinterpret_cast<bool*>(&dataCollectionCommand.exec);
    mbusCoilData.link(boolPtr, 1);
    boolPtr = reinterpret_cast<bool*>(&cmsRestartCommand.restart);
    mbusCoilData.link(boolPtr, 2);
    boolPtr = reinterpret_cast<bool*>(&rmsRestartCommand.restart);
    mbusCoilData.link(boolPtr, 3);
    boolPtr = reinterpret_cast<bool*>(&manualOverride);
    mbusCoilData.link(boolPtr, 4);
    boolPtr = reinterpret_cast<bool*>(&alarmCommand.buzzer);
    mbusCoilData.link(boolPtr, 5);
    boolPtr = reinterpret_cast<bool*>(&alarmCommand.battRelay);
    mbusCoilData.link(boolPtr, 6);
    boolPtr = reinterpret_cast<bool*>(&factoryReset);
    mbusCoilData.link(boolPtr, 7);
    boolPtr = reinterpret_cast<bool*>(&forceBuzzer);
    mbusCoilData.link(boolPtr, 8);
    boolPtr = reinterpret_cast<bool*>(&forceRelay);
    mbusCoilData.link(boolPtr, 9);
    boolPtr = reinterpret_cast<bool*>(&testDataCollection.exec);
    mbusCoilData.link(boolPtr, 10);

    modbusRegisterData.inputRegister.packedData = &packedData;
    modbusRegisterData.inputRegister.otherInfo = &otherInfo;
    modbusRegisterData.holdingRegisters.settingRegisters = &settingRegisters;
    modbusRegisterData.mbusCoil.mbusCoilData = &mbusCoilData;

    startButton.attachClick(buttonClicked);
    startButton.attachLongPressStart(buttonLongPressed);
    // startButton.attachDoubleClick(buttonDoubleClicked);
    startButton.setDebounceTicks(50);
    myTimer = timerBegin(0, 80, true);
    timerAttachInterrupt(myTimer, &onTimer, true);
    timerAlarmWrite(myTimer, 500000, true);
    pinMode(battRelay, OUTPUT);
    pinMode(buzzer, OUTPUT);
    pinMode(internalLed, OUTPUT);
    pinMode(addressOut, OUTPUT);
    digitalWrite(addressOut, HIGH);
    
    Serial2.setRxBufferSize(1024);
    Serial2.begin(115200);
    talis.onDataHandler(onDataCb);
    talis.onErrorHandler(onErrorCb);
    talis.begin(&Serial2, true, '\n');
    // talis.begin(&Serial2);
    talis.setTimeout(500);
    talis.disableLogOutput();

    WiFi.disconnect(true);
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.mode(WIFI_MODE_NULL);
    delay(100);
    WiFi.setHostname("ESP32-Talis");  
    
    WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    // WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    WifiParams wifiParams;

    wifiParams.mode = talisMemory.getMode();

    wifiParams.params.server = talisMemory.getServer();
    strcpy(wifiParams.params.ssid.data(), talisMemory.getSsid().c_str());
    strcpy(wifiParams.params.pass.data(), talisMemory.getPass().c_str());
    strcpy(wifiParams.params.ip.data(), talisMemory.getIp().c_str());
    strcpy(wifiParams.params.gateway.data(), talisMemory.getGateway().c_str());
    strcpy(wifiParams.params.subnet.data(), talisMemory.getSubnet().c_str());

    wifiParams.softApParams.server = server_type::STATIC;
    strcpy(wifiParams.softApParams.ssid.data(), params.ssid.c_str());
    strcpy(wifiParams.softApParams.pass.data(), params.pass.c_str());
    strcpy(wifiParams.softApParams.ip.data(), "192.168.4.1");
    strcpy(wifiParams.softApParams.gateway.data(), "192.168.4.1");
    strcpy(wifiParams.softApParams.subnet.data(), "255.255.255.0");
    
    wifiSetting.begin(wifiParams);

    int timeout = 0;
    
    if (wifiSetting.getMode() == mode_type::STATION || wifiSetting.getMode() == mode_type::AP_STATION)
    {
        while (WiFi.status() != WL_CONNECTED)
        {
            if (timeout >= 10)
            {
                Serial.println("Failed to connect into " + wifiSetting.getSsid());
                break;
            }
            Serial.print(".");
            delay(500);
            timeout++;
        }
    }
    
    if (wifiSetting.getMode() == mode_type::AP || wifiSetting.getMode() == mode_type::AP_STATION)
    {
        ESP_LOGI(TAG, "AP Up");
        ESP_LOGI(TAG, "SSID : %s\n", WiFi.softAPSSID().c_str());
        ESP_LOGI(TAG, "IP address: %s\n", WiFi.softAPIP().toString().c_str());
        ESP_LOGI(TAG, "Subnet Mask: %s\n", WiFi.softAPSubnetMask().toString().c_str());
        ESP_LOGI(TAG, "Hostname: %s\n", WiFi.softAPgetHostname());
        if (wifiSetting.getMode() == mode_type::AP)
        {
            digitalWrite(internalLed, HIGH);
        }
    }

    delay(100); //wait a bit to stabilize voltage and current
    if (!MDNS.begin(rackSn)) {             // Start the mDNS responder for esp8266.local
        Serial.println("Error setting up MDNS responder!");
    }
    Serial.println("mDNS responder started");
    delay(100);

    if (timeout >= 10)
    {
        Serial.println("WiFi Not Connected");
        digitalWrite(internalLed, LOW);
    }
    
    timerAlarmEnable(myTimer);
    delay(100); //wait a bit to stabilize the voltage and current consumption
    digitalWrite(battRelay, HIGH);
    #ifdef HARDWARE_ALARM
        hardwareAlarmEnable = true;
    #endif
    declareStruct();
    ledAnimation.setLedGroupNumber(addressList.size());
    ledAnimation.setLedStringNumber(8);
    ledAnimation.run();

    ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    //   Serial.println("Start updating " + type);
      ESP_LOGI(TAG, "Start updating %s\n", type.c_str());
    })
    .onEnd([]() {
    //   Serial.println("\nEnd");
        ESP_LOGI(TAG, "\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
    //   Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    ESP_LOGI(TAG, "Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      ESP_LOGI(TAG, "Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) ESP_LOGI(TAG, "Auth Failed");
      else if (error == OTA_BEGIN_ERROR) ESP_LOGI(TAG, "Begin Failed");
      else if (error == OTA_CONNECT_ERROR) ESP_LOGI(TAG, "Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) ESP_LOGI(TAG, "Receive Failed");
      else if (error == OTA_END_ERROR) ESP_LOGI(TAG, "End Failed");
    });

    ArduinoOTA.begin();

    // server.serveStatic("/", LittleFS, "/page1.html");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html");
    });

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/update.html", "text/html");
    });

    server.on("/info", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/info.html", "text/html");
    });

    server.on("/assets/progressUpload.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/assets/progressUpload.js", "application/javascript");
    });

    server.on("/assets/function_without_ajax.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/assets/function_without_ajax.js", "application/javascript");
    });

    server.on("/assets/function_info_page.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/assets/function_info_page.js", "application/javascript");
    });

    server.on("/assets/index.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/assets/index.css", "text/css");
    });

    server.on("/assets/info.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/assets/info.css", "text/css");
    });

    server.on("/assets/update_styles.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/assets/update_styles.css", "text/css");
    });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/assets/favicon.ico", "image/x-icon");
    });

    server.on("/get-device-info", HTTP_GET, [](AsyncWebServerRequest *request){
        String output;
        StaticJsonDocument<256> doc;
        doc["firmware_version"] = VERSION;
        
        if (wifiSetting.getMode() == mode_type::STATION || wifiSetting.getMode() == mode_type::AP_STATION)
        {
            doc["device_ip"] = WiFi.localIP().toString();
            doc["ssid"] = WiFi.SSID();
            switch (wifiSetting.getStationServer())
            {
            case server_type::DHCP :
                doc["server_mode"] = "DHCP";
                break;
            case server_type::STATIC :
                doc["server_mode"] = "DHCP";
                break;
            default:
                break;
            }
        }
        else
        {
            doc["device_ip"] = WiFi.softAPIP().toString();
            doc["ssid"] = WiFi.softAPSSID();
            switch (wifiSetting.getApServer())
            {
            case server_type::DHCP :
                doc["server_mode"] = "DHCP";
                break;
            case server_type::STATIC :
                doc["server_mode"] = "DHCP";
                break;
            default:
                break;
            }
        }
        doc["mac_address"] = mac;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
    //     request->send(200, "text/plain", "Talis 30 MJ Rack Management System"); });

    server.on("/get-single-cms-data", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        int bid = jsonManager.processSingleCmsDataRequest(request);
        String jsonOutput;
        if (cmsData.find(bid) != cmsData.end())
        {
            jsonOutput = jsonManager.buildSingleJsonData(cmsData[bid]);
        }
        else
        {
            StaticJsonDocument<16> doc;
            serializeJson(doc, jsonOutput);
        }
        request->send(200, "application/json", jsonOutput);
    });

    server.on("/get-cms-data", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        // String jsonOutput = jsonManager.buildJsonData(request, cellData, cellDataArrSize);
        String jsonOutput = jsonManager.buildJsonData(request, packedData);
        request->send(200, "application/json", jsonOutput); });

    server.on("/get-data", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        String buffer;
        if(jsonManager.buildJsonData(request, packedData, buffer))
        {
            request->send(200, "application/json", buffer);
        }
        else
        {
            request->send(400);
        }
    });

    server.on("/get-device-general-info", HTTP_GET, [](AsyncWebServerRequest *request)
    { 
        RMSInfo rmsInfo;
        rmsInfo.rackSn = rackSn;
        rmsInfo.rmsCode = rmsCode;     
        switch (wifiSetting.getMode())
        {
            case mode_type::STATION :
                rmsInfo.ip = WiFi.localIP().toString();
            break;
            case mode_type::AP :
                rmsInfo.ip = WiFi.softAPIP().toString();
            break;
            case mode_type::AP_STATION :
                rmsInfo.ip = WiFi.localIP().toString();
            break;
            default:
            break;
        }
        rmsInfo.mac = WiFi.macAddress();
        rmsInfo.ver = VERSION;
        rmsInfo.deviceTypeName = "RMS";
        String jsonOutput = jsonManager.buildJsonRMSInfo(rmsInfo);
        request->send(200, "application/json", jsonOutput); });

    server.on("/get-balancing-status", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        String jsonOutput = jsonManager.buildJsonBalancingStatus(cellBalanceState);
        request->send(200, "application/json", jsonOutput); });

    server.on("/get-alarm-parameter", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        String jsonOutput = jsonManager.buildJsonAlarmParameter(alarmParam);
        request->send(200, "application/json", jsonOutput); });

    server.on("/get-command-status", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        CommandStatus commandStatus;
        commandStatus.addrCommand = isAddressing;
        commandStatus.dataCollectionCommand = dataCollectionCommand.exec;
        commandStatus.restartCms = cmsRestartCommand.restart;
        commandStatus.restartRms = rmsRestartCommand.restart;
        commandStatus.manualOverride = manualOverride;
        commandStatus.buzzer = alarmCommand.buzzer;
        commandStatus.relay = alarmCommand.battRelay;
        commandStatus.factoryReset = factoryReset;
        commandStatus.buzzerForce = forceBuzzer;
        commandStatus.relayForce = forceRelay; 
        String jsonOutput = jsonManager.buildJsonCommandStatus(commandStatus);
        request->send(200, "application/json", jsonOutput); });

    server.on("/get-addressing-status", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        AddressingStatus addressingStatus;
        addressingStatus.numOfDevice = addressList.size();
        Serial.println("Number of Device : " + String(addressingStatus.numOfDevice));
        for (int i = 0; i < addressList.size(); i++)
        {
            addressingStatus.deviceAddressList[i] = addressList.at(i);
            Serial.println(addressingStatus.deviceAddressList[i]);
        }
        addressingStatus.status = isAddressed;
        Serial.println("Addressing Completed Flag : " + String(addressingStatus.status));
        String jsonOutput = jsonManager.buildJsonAddressingStatus(addressingStatus, addressList.size());
        request->send(200, "application/json", jsonOutput); });

    server.on("/get-network-info", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      Serial.println("Get connected network info");
      NetworkSetting s;
      switch (wifiSetting.getMode())
      {
        case mode_type::STATION :
            s.ssid = WiFi.SSID();
            s.ip = WiFi.localIP().toString();
        break;
        case mode_type::AP :
            s.ssid = WiFi.softAPSSID();
            s.ip = WiFi.softAPIP().toString();
        break;
        case mode_type::AP_STATION :
            s.ssid = WiFi.SSID();
            s.ip = WiFi.localIP().toString();
        break;
        default:
        break;
      }
      request->send(200, "application/json", jsonManager.getNetworkInfo(s)); });

    server.on("/get-user-network-setting", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      Serial.println("Get user network setting info");
      NetworkSetting s;
      s.ssid = talisMemory.getSsid();
      s.pass = talisMemory.getPass();
      s.ip = talisMemory.getIp();
      s.gateway = talisMemory.getGateway();
      s.subnet = talisMemory.getSubnet();
      s.mode = talisMemory.getMode();
      s.server = talisMemory.getServer();
    //   Preferences preferences;    
    //   preferences.begin("dev_params");
    //   s.ssid = preferences.getString("ssid");
    //   s.pass = preferences.getString("pass");
    //   s.ip = preferences.getString("ip");
    //   s.gateway = preferences.getString("gateway");
    //   s.subnet = preferences.getString("subnet");
    //   s.mode = preferences.getChar("mode");
    //   s.server = preferences.getChar("server");
    //   preferences.end();
      request->send(200, "application/json", jsonManager.getUserNetworkSetting(s)); });

    server.on("/get-user-alarm-setting", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        Serial.println("Get user alarm setting info");      
        AlarmParam alm;
        alm.vcell_diff = talisMemory.getCellDifference();
        alm.vcell_diff_reconnect = talisMemory.getCellDifferenceReconnect();
        alm.vcell_overvoltage = talisMemory.getCellOvervoltage();
        alm.vcell_undervoltage = talisMemory.getCellUndervoltage();
        alm.vcell_reconnect = talisMemory.getCellUndervoltageReconnect();
        alm.temp_max = talisMemory.getCellOvertemperature();
        alm.temp_min = talisMemory.getCellUndertemperature();
        request->send(200, "application/json", jsonManager.getUserAlarmSetting(alm)); });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404);
    });

    server.on("/update-firmware", HTTP_POST, [](AsyncWebServerRequest *request){
        String output;
        int code = 200;
        StaticJsonDocument<16> doc;
        doc["status"] = code;
        serializeJson(doc, output);
        request->send(200, "text/plain", "File has been uploaded successfully.");
        rmsRestartCommand.restart = true;
        // lastTime = millis();
    }, handleFirmwareUpload);

    server.on("/update-compressed-firmware", HTTP_POST, [](AsyncWebServerRequest *request){
        String output;
        int code = 200;
        StaticJsonDocument<16> doc;
        doc["status"] = code;
        serializeJson(doc, output);
        request->send(200, "text/plain", "File has been uploaded successfully.");
        rmsRestartCommand.restart = true;
    }, handleCompressedFirmwareUpload);

    AsyncCallbackJsonWebHandler *setBalancingHandler = new AsyncCallbackJsonWebHandler("/set-balancing", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        int status = jsonManager.jsonBalancingCommandParser(input.c_str(), cellBalancingCommand);
        doc["status"] = status;
        serializeJson(doc, response);
        response.replace(":status:", String(status));
        if (status >= 0)
        {
            if (status > 0)
            {
                TalisRS485TxMessage txMsg;
                txMsg.token = TalisRS485::RequestType::BALANCINGWRITE;
                txMsg.id = cellBalancingCommand.bid;
                TalisRS485Message::createCMSWriteBalancingRequest(txMsg, cellBalancingCommand.cball, 45);
                userCommand.push_back(txMsg);
            }
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        });

    AsyncCallbackJsonWebHandler *setAddressHandler = new AsyncCallbackJsonWebHandler("/set-addressing", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        int status = jsonManager.jsonAddressingCommandParser(input.c_str());
        doc["status"] = status;
        serializeJson(doc, response);
        if (status >= 0)
        {
            if (status)
            {
                isAddressing = true;
                lastCheckQueue = millis();
            }
            else
            {
                isAddressing = false;
            }
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        });

    AsyncCallbackJsonWebHandler *setAlarmHandler = new AsyncCallbackJsonWebHandler("/set-alarm", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        int status = jsonManager.jsonAlarmCommandParser(input.c_str(), alarmCommand);
        doc["status"] = status;
        serializeJson(doc, response);
        request->send(200, "application/json", response); });

    AsyncCallbackJsonWebHandler *setDataCollectionHandler = new AsyncCallbackJsonWebHandler("/set-data-collection", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        int status = jsonManager.jsonDataCollectionCommandParser(input.c_str());
        doc["status"] = status;
        serializeJson(doc, response);
        dataCollectionCommand.exec = status;
        request->send(200, "application/json", response); });
    
    AsyncCallbackJsonWebHandler *setLedHandler = new AsyncCallbackJsonWebHandler("/set-led", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        LedCommand ledCommand;
        int status = jsonManager.jsonLedParser(input.c_str(), ledCommand);
        doc["status"] = status;
        serializeJson(doc, response);
        if (status >= 0)
        {
            if (status > 0)
            {
                TalisRS485TxMessage txMsg;
                txMsg.token = TalisRS485::RequestType::LED;
                txMsg.id = ledCommand.bid;
                LedColor ledColor[8];
                for (size_t i = 0; i < ledCommand.num_of_led; i++)
                {
                    ledColor[i].red = ledCommand.red[i];
                    ledColor[i].green = ledCommand.green[i];
                    ledColor[i].blue = ledCommand.blue[i];
                }
                TalisRS485Message::createCMSWriteLedRequest(txMsg, ledColor, ledCommand.num_of_led);
                userCommand.push_back(txMsg);
            }
            
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        });

    AsyncCallbackJsonWebHandler *setAlarmParamHandler = new AsyncCallbackJsonWebHandler("/set-alarm-parameter", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        int status = jsonManager.jsonAlarmParameterParser(input.c_str(), alarmParam);
        doc["status"] = status;
        serializeJson(doc, response);
        if (status > 0)
        {
            Serial.println("Set parameter");
            talisMemory.setCellDifference(alarmParam.vcell_diff);
            talisMemory.setCellDifferenceReconnect(alarmParam.vcell_diff_reconnect);
            talisMemory.setCellOvervoltage(alarmParam.vcell_overvoltage);
            talisMemory.setCellUndervoltage(alarmParam.vcell_undervoltage);
            talisMemory.setCellUndervoltageReconnect(alarmParam.vcell_reconnect);
            talisMemory.setCellOvertemperature(alarmParam.temp_max);
            talisMemory.setCellUndertemperature(alarmParam.temp_min);
            request->send(200, "application/json", response);    
        }
        else {
            request->send(400, "application/json", response);
        } });
        

    AsyncCallbackJsonWebHandler *setHardwareAlarmHandler = new AsyncCallbackJsonWebHandler("/set-hardware-alarm", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        int status = jsonManager.jsonHardwareAlarmEnableParser(input.c_str(), hardwareAlarmEnable);
        doc["status"] = status;
        serializeJson(doc, response);
        if (status >= 0)
        {
            request->send(200, "application/json", response);    
        }
        else
        {
            request->send(400, "application/json", response);
        }
        });

    AsyncCallbackJsonWebHandler *setSleepHandler = new AsyncCallbackJsonWebHandler("/set-sleep", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        CMSShutDown cmsShutDown;
        int status = jsonManager.jsonCMSShutdownParser(input.c_str(), cmsShutDown);
        doc["status"] = status;
        serializeJson(doc, response);
        cmsShutDown.shutdown = status;
        if (status >= 0)
        {
            if (status > 0)
            {
                TalisRS485TxMessage txMsg;
                txMsg.token = TalisRS485::RequestType::SHUTDOWN;
                txMsg.id = cmsShutDown.bid;
                TalisRS485Message::createShutDownRequest(txMsg);
                userCommand.push_back(txMsg);
            }
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        
        });

    AsyncCallbackJsonWebHandler *setWakeupHandler = new AsyncCallbackJsonWebHandler("/set-wakeup", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        CMSWakeup cmsWakeup;
        int status = jsonManager.jsonCMSWakeupParser(input.c_str(), cmsWakeup);
        doc["status"] = status;
        serializeJson(doc, response);
        cmsWakeup.wakeup = status;
        if (status >= 0)
        {
            if (status > 0)
            {
                TalisRS485TxMessage txMsg;
                txMsg.token = TalisRS485::RequestType::WAKEUP;
                txMsg.id = cmsWakeup.bid;
                TalisRS485Message::createWakeupRequest(txMsg);
                userCommand.push_back(txMsg);
            }
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        });
    
    AsyncCallbackJsonWebHandler *restartCMSHandler = new AsyncCallbackJsonWebHandler("/restart-cms", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        CMSRestartCommand cmd;
        int status = jsonManager.jsonCMSRestartParser(input.c_str(), cmd);
        doc["status"] = status;
        serializeJson(doc, response);
        if (status >= 0)
        {
            if (status > 0)
            {
                // addressList.clear();
                // TalisRS485TxMessage txMsg;
                // txMsg.id = cmd.bid;
                // TalisRS485Message::createCMSResetRequest(txMsg);
                // txMsg.token = TalisRS485::RequestType::RESTART;
                // userCommand.push_back(txMsg);
                isAddressing = true;
                lastCheckQueue = millis();
            }
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        });

    AsyncCallbackJsonWebHandler *restartRMSHandler = new AsyncCallbackJsonWebHandler("/restart", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        int status = jsonManager.jsonRMSRestartParser(input.c_str());
        doc["status"] = status;
        serializeJson(doc, response);
        if (status >= 0)
        {
            if (status > 0)
            {
                rmsRestartCommand.restart = true;
            }
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        });

    AsyncCallbackJsonWebHandler *setRmsCodeHandler = new AsyncCallbackJsonWebHandler("/set-rms-code", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        MasterWrite masterWrite;
        int status = jsonManager.jsonRmsCodeParser(input.c_str(), masterWrite);
        doc["status"] = status;
        serializeJson(doc, response);
        if (status >= 0)
        {
            if (status > 0)
            {
                bool success = talisMemory.setRmsName(masterWrite.content.c_str());
                if (success)
                {
                    rmsCode = talisMemory.getRmsName();
                }
            }
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        });

    AsyncCallbackJsonWebHandler *setRackSnHandler = new AsyncCallbackJsonWebHandler("/set-rack-sn", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        MasterWrite masterWrite;
        int status = jsonManager.jsonRmsRackSnParser(input.c_str(), masterWrite);
        doc["status"] = status;
        serializeJson(doc, response);
        if (status >= 0)
        {
            if (status)
            {
                bool success = talisMemory.setRackSn(masterWrite.content.c_str());
                if (success)
                {
                    rackSn = talisMemory.getRackSn();
                }
            }
            request->send(400, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        });

    AsyncCallbackJsonWebHandler *setFrameHandler = new AsyncCallbackJsonWebHandler("/set-frame", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        SlaveWrite slaveWrite;
        int status = jsonManager.jsonCMSFrameParser(input.c_str(), slaveWrite);
        doc["status"] = status;
        serializeJson(doc, response);
        if (status >= 0)
        {
            if (status > 0)
            {
                TalisRS485TxMessage txMsg;
                txMsg.token = TalisRS485::RequestType::CMSFRAMEWRITE;
                txMsg.id = slaveWrite.bid;
                TalisRS485Message::createCMSFrameWriteIdRequest(txMsg, slaveWrite.content);
                userCommand.push_back(txMsg);
            }
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        });

    AsyncCallbackJsonWebHandler *setCmsCodeHandler = new AsyncCallbackJsonWebHandler("/set-cms-code", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        SlaveWrite slaveWrite;
        int status = jsonManager.jsonCMSCodeParser(input.c_str(), slaveWrite);
        doc["status"] = status;
        serializeJson(doc, response);
        if (status >= 0)
        {
            if (status > 0)
            {
                TalisRS485TxMessage txMsg;
                txMsg.token = TalisRS485::RequestType::CMSCODEWRITE;
                txMsg.id = slaveWrite.bid;
                TalisRS485Message::createCMSCodeWriteRequest(txMsg, slaveWrite.content);
                userCommand.push_back(txMsg);
            }
            request->send(200, "application/json", response);    
        }
        else
        {
            request->send(400, "application/json", response);
        }
        
        });

    AsyncCallbackJsonWebHandler *setBaseCodeHandler = new AsyncCallbackJsonWebHandler("/set-base-code", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        SlaveWrite slaveWrite;
        int status = jsonManager.jsonCMSBaseCodeParser(input.c_str(), slaveWrite);
        doc["status"] = status;
        serializeJson(doc, response);
        if (status >= 0)
        {
            if (status > 0)
            {
                TalisRS485TxMessage txMsg;
                txMsg.token = TalisRS485::RequestType::CMSBASECODEWRITE;
                txMsg.id = slaveWrite.bid;
                TalisRS485Message::createCMSBaseCodeWriteRequest(txMsg, slaveWrite.content);
                userCommand.push_back(txMsg);
            }
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        });

    AsyncCallbackJsonWebHandler *setMcuCodeHandler = new AsyncCallbackJsonWebHandler("/set-mcu-code", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        SlaveWrite slaveWrite;
        int status = jsonManager.jsonCMSMcuCodeParser(input.c_str(), slaveWrite);
        doc["status"] = status;
        serializeJson(doc, response);
        if (status >= 0)
        {
            if (status > 0)
            {
                TalisRS485TxMessage txMsg;
                txMsg.token = TalisRS485::RequestType::CMSMCUCODEWRITE;
                txMsg.id = slaveWrite.bid;
                TalisRS485Message::createCMSMcuCodeWriteRequest(txMsg, slaveWrite.content);
                userCommand.push_back(txMsg);
            }
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        });

    AsyncCallbackJsonWebHandler *setSiteLocationHandler = new AsyncCallbackJsonWebHandler("/set-site-location", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String input = json.as<String>();
        StaticJsonDocument<16> doc;
        String response;
        SlaveWrite slaveWrite;
        int status = jsonManager.jsonCMSSiteLocationParser(input.c_str(), slaveWrite);
        doc["status"] = status;
        serializeJson(doc, response);
        if (status >= 0)
        {
            if (status > 0)
            {
                TalisRS485TxMessage txMsg;
                txMsg.token = TalisRS485::RequestType::CMSSITELOCATIONWRITE;
                txMsg.id = slaveWrite.bid;
                TalisRS485Message::createCMSSiteLocationWriteRequest(txMsg, slaveWrite.content);
                userCommand.push_back(txMsg);
            }
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        });

    AsyncCallbackJsonWebHandler *setNetwork = new AsyncCallbackJsonWebHandler("/set-network", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        StaticJsonDocument<16> doc;
        String response;
        NetworkSetting setting = jsonManager.parseNetworkSetting(json);
        doc["status"] = setting.flag;
        serializeJson(doc, response);
        if (setting.flag > 0)
        {
            Serial.println("Set network");
            size_t resultLength = Utilities::toDoubleChar(setting.ssid, ssidArr, 8, true);
            // Serial.println(resultLength);
            resultLength = Utilities::toDoubleChar(setting.pass, passArr, 8, true);
            // Serial.println(resultLength);
            IPAddress ip;
            IPAddress gateway;
            IPAddress subnet;
            ip.fromString(setting.ip);
            gateway.fromString(setting.gateway);
            subnet.fromString(setting.subnet);
            for (size_t i = 0; i < 4; i++)
            {
                ipOctet[i] = ip[i];
                // Serial.println(ipOctet[i]);
                gatewayOctet[i] = gateway[i];
                // Serial.println(gatewayOctet[i]);
                subnetOctet[i] = subnet[i];
                // Serial.println(subnetOctet[i]);
            }

            gServerType = wifiSetting.getStationServer();
            gMode = wifiSetting.getMode();
            
            talisMemory.setSsid(setting.ssid.c_str());
            talisMemory.setPass(setting.pass.c_str());
            talisMemory.setIp(setting.ip.c_str());
            talisMemory.setGateway(setting.gateway.c_str());
            talisMemory.setSubnet(setting.subnet.c_str());
            talisMemory.setServer(setting.server);
            talisMemory.setMode(setting.mode);

            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
    });

    AsyncCallbackJsonWebHandler *setFactoryReset = new AsyncCallbackJsonWebHandler("/set-factory-reset", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        StaticJsonDocument<16> doc;
        String response;
        int status = jsonManager.parseFactoryReset(json);
        doc["status"] = status;
        serializeJson(doc, response);
        if (status == 1 || status == 0)
        {
            if (status)
            {
                factoryReset = 1;
            }
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        
    });

    AsyncCallbackJsonWebHandler *setSoc = new AsyncCallbackJsonWebHandler("/set-soc", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        
        String input = json.as<String>();
        StaticJsonDocument<32> docInput;

        DeserializationError error = deserializeJson(docInput, input);

        if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
            return;
        }

        int soc = docInput["soc"]; // 1

        TalisRS485Utils::socToLed(4, 8, soc);

        StaticJsonDocument<16> doc;
        String response;
        int status = 1;
        doc["status"] = status;
        serializeJson(doc, response);
        if (status == 1 || status == 0)
        {
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        
    });

    server.addHandler(setBalancingHandler);
    server.addHandler(setAddressHandler);
    server.addHandler(setAlarmHandler);
    server.addHandler(setDataCollectionHandler);
    server.addHandler(setAlarmParamHandler);
    server.addHandler(setHardwareAlarmHandler);
    server.addHandler(setSleepHandler);
    server.addHandler(setWakeupHandler);
    server.addHandler(setRmsCodeHandler);
    server.addHandler(setRackSnHandler);
    server.addHandler(setFrameHandler);
    server.addHandler(setCmsCodeHandler);
    server.addHandler(setBaseCodeHandler);
    server.addHandler(setMcuCodeHandler);
    server.addHandler(setSiteLocationHandler);
    server.addHandler(setLedHandler);
    server.addHandler(restartCMSHandler);
    server.addHandler(restartRMSHandler);
    server.addHandler(setNetwork);
    server.addHandler(setFactoryReset);
    server.addHandler(setSoc);

    MBserver.registerWorker(1, READ_COIL, &FC01);      // FC=01 for serverID=1
    MBserver.registerWorker(1, READ_HOLD_REGISTER, &FC03);      // FC=03 for serverID=1
    MBserver.registerWorker(1, READ_INPUT_REGISTER, &FC04);     // FC=04 for serverID=1
    MBserver.registerWorker(1, WRITE_COIL, &FC05);     // FC=05 for serverID=1
    MBserver.registerWorker(1, WRITE_HOLD_REGISTER, &FC06);      // FC=06 for serverID=1
    MBserver.registerWorker(1, WRITE_MULT_REGISTERS, &FC16);    // FC=16 for serverID=1

    server.begin();
    Serial.println("HTTP server started");
    lastReconnectMillis = millis();
    dataCollectionCommand.exec = false;
    MBserver.start(502, 10, 20000);
    systemStatus.bits.ready = 1;
    testDataCollection.exec = 1;
    isAddressing = true;
    lastErrorCounterCheck = millis();
    lastCheckQueue = millis();
    ESP_LOGI(TAG, "================== NEW PROGRAM 4 ==================");
}

void loop()
{    
    if (wifiSetting.getMode() == mode_type::STATION || wifiSetting.getMode() == mode_type::AP_STATION)
    {
        if ((WiFi.status() != WL_CONNECTED) && (millis() - lastReconnectMillis >= reconnectInterval)) {
            digitalWrite(internalLed, LOW);
            ESP_LOGI(TAG, "===============Reconnecting to WiFi...========================\n");
            if (WiFi.reconnect())
            {
                lastReconnectMillis = millis();
            }
        }
    }

    if (manualOverride)
    {
        digitalWrite(battRelay, forceRelay);
        if (!forceBuzzer)
        {
            digitalWrite(buzzer, LOW);
        }
    }
    else
    {
        if (alarmCommand.buzzer)
        {
            digitalWrite(battRelay, LOW);
        }
        else
        {
            digitalWrite(buzzer, LOW);
            digitalWrite(battRelay, HIGH);
        }
    }

    if (factoryReset)
    {
        talisMemory.reset();
        ESP_LOGI(TAG, "Factory Reset");
        rmsRestartCommand.restart = true;
        factoryReset = false;
    }

    if (rmsRestartCommand.restart)
    {
        rmsRestartCommand.restart = false;
        digitalWrite(internalLed, LOW);
        delay(100);
        ESP.restart();
    }

    

    // if (cmsRestartCommand.restart)
    // {
    //     addressList.clear();
    //     TalisRS485TxMessage txMsg;
    //     txMsg.id = 255;
    //     TalisRS485Message::createCMSResetRequest(txMsg);
    //     txMsg.token = TalisRS485::RequestType::RESTART;
    //     userCommand.push_back(txMsg);
    //     cmsRestartCommand.restart = false;
    //     isAddressing = true;
    // }


    if(hardwareAlarmEnable)
    {
        // ESP_LOGI(TAG, "Updater element size : %d\n", updater.size());
        if (updater.size() == addressList.size())
        {
            // ESP_LOGI(TAG, "==============Data completed==========");
            bool buzzerState = false;
            bool cellDiffAlm = false;
            bool cellOvervoltage = false;
            bool cellUndervoltage = false;
            bool overtemperature = false;
            bool undertemperature = false;
            std::map<int, Updater>::iterator it;
            std::map<int, CMSData>::iterator cmsIterator;
            for (it = updater.begin(); it != updater.end(); it++)
            {
                if (!(*it).second.isDataNormal())
                {
                    // ESP_LOGI(TAG, "Buzzer turning on");
                    buzzerState = true;
                    break;
                }
            }
            for (cmsIterator = packedData.cms->begin(); cmsIterator != packedData.cms->end(); cmsIterator++)
            {
                if ((*cmsIterator).second.packStatus.bits.cellDiffAlarm)
                {
                    // ESP_LOGI(TAG, "Cell difference alarm on first bid : %d\n", (*cmsIterator).second.bid);
                    cellDiffAlm = true;
                    break;
                }
            }
            for (cmsIterator = packedData.cms->begin(); cmsIterator != packedData.cms->end(); cmsIterator++)
            {
                if ((*cmsIterator).second.packStatus.bits.cellOvervoltage)
                {
                    // ESP_LOGI(TAG, "Cell overvoltage alarm on first bid : %d\n", (*cmsIterator).second.bid);
                    cellOvervoltage = true;
                    break;
                }
            }
            for (cmsIterator = packedData.cms->begin(); cmsIterator != packedData.cms->end(); cmsIterator++)
            {
                if ((*cmsIterator).second.packStatus.bits.cellUndervoltage)
                {
                    // ESP_LOGI(TAG, "Cell undervoltage alarm on first bid : %d\n", (*cmsIterator).second.bid);
                    cellUndervoltage = true;
                    break;
                }
            }
            for (cmsIterator = packedData.cms->begin(); cmsIterator != packedData.cms->end(); cmsIterator++)
            {
                if ((*cmsIterator).second.packStatus.bits.overtemperature)
                {
                    // ESP_LOGI(TAG, "Cell overtemperature alarm on first bid : %d\n", (*cmsIterator).second.bid);
                    overtemperature = true;
                    break;
                }
            }
            for (cmsIterator = packedData.cms->begin(); cmsIterator != packedData.cms->end(); cmsIterator++)
            {
                if ((*cmsIterator).second.packStatus.bits.undertemperature)
                {
                    // ESP_LOGI(TAG, "Cell undertemperature alarm on first bid : %d\n", (*cmsIterator).second.bid);
                    undertemperature = true;
                    break;
                }
            }
            systemStatus.bits.cellDiffAlarm = cellDiffAlm;
            systemStatus.bits.cellOvervoltage = cellOvervoltage;
            systemStatus.bits.cellUndervoltage = cellUndervoltage;
            systemStatus.bits.overtemperature = overtemperature;
            systemStatus.bits.undertemperature = undertemperature;
            alarmCommand.buzzer = buzzerState;
        }
    }
    
    if (command > 6)
    {
        command = 0;
        id++;
    }

    if (id >= addressList.size())
    {
        id = 0;
    }
    
    if (isAddressing)
    {
        if (talis.isTxQueueEmpty() && userCommand.empty())
        {
            if (millis() - lastCheckQueue > 2000)
            {
                ESP_LOGI(TAG, "isAddressing");
                isAddressed = false;
                TalisRS485TxMessage txMsg;
                txMsg.id = 255;
                txMsg.requestCode = TalisRS485::RequestType::RESTART;
                txMsg.token = 10000;
                TalisRS485Message::createCMSResetRequest(txMsg);
                talis.send(txMsg);
                digitalWrite(addressOut, HIGH); // need to be placed here or else addressing will be failed
                delay(2000);
                beginAddressing = true;
                ESP_LOGI(TAG, "reset cms");
                lastCheckQueue = millis();
                cmsData.clear();
            }
        }
        else
        {
            lastCheckQueue = millis();
        }
    }
    // while(1);
    if (!beginAddressing)
    {
        // check for error counter
        if (millis() - lastErrorCounterCheck > 3000)
        {
            std::map<int, CMSData>::iterator it;
            std::map<int, CMSData> buffer = cmsData;

            for (it = buffer.begin(); it != buffer.end(); it++)
            {
                if ((*it).second.errorCount > 3)
                {
                    ESP_LOGI(TAG, "Perform re-address");
                    isAddressing = true;
                    lastCheckQueue = millis(); // reset check queue timer
                    break;
                } 
            }
            lastErrorCounterCheck = millis(); // reset error counter check timer
        }
        
        if (userCommand.size() > 0)
        {
            // ESP_LOGI(TAG, "User command size : %d\n", userCommand.size());
            TalisRS485::Error error = talis.addRequest(userCommand.at(0));
            if (error == TalisRS485::Error::SUCCESS)
            {
                userCommand.erase(userCommand.begin());
            }  
        }
        else
        {
            if (millis() - lastSent > 100)
            {
                if (!addressList.empty()) // scheduler command building
                {
                    // ESP_LOGI(TAG, "address list size : %d\n", addressList.size());
                    TalisRS485TxMessage txMsg;
                    txMsg.id = addressList.at(id);
                    txMsg.token = commandOrder[command];
                    switch (commandOrder[command])
                    {
                        case TalisRS485::RequestType::VCELL :
                            TalisRS485Message::createVcellRequest(txMsg);
                        break;
                        case TalisRS485::RequestType::TEMP :
                            TalisRS485Message::createTemperatureRequest(txMsg);
                        break;
                        case TalisRS485::RequestType::VPACK :
                            TalisRS485Message::createVpackRequest(txMsg);
                        break;
                        case TalisRS485::RequestType::CMSSTATUS :
                            TalisRS485Message::createCMSStatusRequest(txMsg);
                        break;
                        case TalisRS485::RequestType::CMSREADBALANCINGSTATUS :
                            TalisRS485Message::createReadBalancingStatus(txMsg);
                        break;
                        case TalisRS485::RequestType::CMSINFO :
                            TalisRS485Message::createCMSInfoRequest(txMsg);
                        break;
                        case TalisRS485::RequestType::LED :
                            if (ledAnimation.isRunning())
                            {
                                LedData ledData = ledAnimation.getLed();
                                LedColor ledColor[8];
                                ESP_LOGI(TAG, "Group : %d\n", ledData.currentGroup);
                                ESP_LOGI(TAG, "String : %d\n", ledData.currentString);
                                for (size_t i = 0; i < 8; i++)
                                {
                                    ledColor[i].red = ledData.red[i];
                                    ledColor[i].green = ledData.green[i];
                                    ledColor[i].blue = ledData.blue[i];
                                }
                                if(ledData.currentGroup >= 0)
                                {
                                    txMsg.id = ledData.currentGroup+1; //current group start from 0, while bid start from 1 so it needs to be offseted
                                }
                                TalisRS485Message::createCMSWriteLedRequest(txMsg, ledColor, 8);
                            }
                        break;
                        default:
                        break;
                    }
                    
                    if (!isAddressing) // push built command into queue
                    {
                        // ESP_LOGV(TAG, "ID : %d\nCommand : %d\n", id, command);
                        TalisRS485::Error error = talis.addRequest(txMsg);
                        if (error == TalisRS485::Error::SUCCESS)
                        {
                            command++;
                            lastSent = millis();
                        }
                        else
                        {
                            ESP_LOGI(TAG, "Failed to add request");
                        }                        
                    }
                }
            }
        }
    }  
    else
    {
        ESP_LOGI(TAG, "Addressing in waiting state..");
        ESP_LOGI(TAG, "Addressing in progress..");
        bool isFromBottom = true;
        talis.pause();
        talis.setTimeout(100);
        currentAddrBid = 1;
        addressList.clear();
        addressing(true, numOfShiftRegister);
        talis.setTimeout(500);
        talis.resume();
        isAddressing = false;
        beginAddressing = false;
        ledAnimation.setLedGroupNumber(addressList.size());
        ESP_LOGI(TAG, "Addressing is finished");
        isAddressed = true;
        lastErrorCounterCheck = millis();
        lastCheckQueue = millis();
        // talis.resume();
    }
    delay(1);
} // void loop end