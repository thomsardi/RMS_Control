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
#include "defs.h"

#define EEPROM_RMS_CODE_ADDRESS 0x00    //Address for RMS Code Sn
#define EEPROM_RMS_ADDRESS_CONFIGURED_FLAG 0x20 //Address for configured flag

#define EEPROM_RACK_SN_ADDRESS 0x30    //Address for Rack Serial Number
#define EEPROM_RACK_SN_CONFIGURED_FLAG 0x50 //Address for configured flag

#define HARDWARE_ALARM_ENABLED 0x60 //Address to check hardware enabled alarm

#define RXD2 16
#define TXD2 17

#define I2C_SDA 33
#define I2C_SCL 32

#define LED_PIN 27

#define NUM_LEDS 10

#define USE_BQ76940 1

// #define AUTO_POST 1 //comment to disable server auto post

// #define LAMINATE_ROOM 1 //uncomment to use board in laminate room

#define CYCLING 1

#define HARDWARE_ALARM 1

#define DEBUG 1

// #define DEBUG_STATIC 1

#ifdef LAMINATE_ROOM
    // #define SERIAL_DATA 12
    // #define SHCP 14
    // #define STCP 13
    #define SERIAL_DATA 14
    #define SHCP 13
    #define STCP 12
    #define DATABASE_IP "192.168.2.174"
    #define SERVER_NAME "http://192.168.2.174/mydatabase/"
    #define HOST_NAME "RMS-Laminate-Room"
#else
    #define SERIAL_DATA 14
    #define SHCP 13
    #define STCP 12
    #define DATABASE_IP "192.168.2.132"
    #define SERVER_NAME "http://192.168.2.132/mydatabase/"
    #define HOST_NAME "RMS-Rnd-Room"
#endif

const char* TAG = "RMS-Control-Event";

std::vector<TalisRS485TxMessage> userCommand;
std::vector<int> addressList;

QueueHandle_t rs485ReceiverQueue = xQueueCreate(10, sizeof(TalisRS485RxMessage));
QueueHandle_t idUpdate = xQueueCreate(10, sizeof(int));
TaskHandle_t rs485ReceiverTaskHandle;
TaskHandle_t rs485TransmitterTaskHandle;
TaskHandle_t counterUpdater;

int battRelay = 23;
int buzzer = 26;

int internalLed = 2;

int const numOfShiftRegister = 8;

// int serialData = 12;
// int shcp = 14;
// int stcp = 13;

int lcdColumns = 16;
int lcdRows = 2;

uint8_t ledDIN = 27;

ShiftRegister74HC595<numOfShiftRegister> sr(SERIAL_DATA, SHCP, STCP);
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

CRGB leds[NUM_LEDS];

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

const char *host = DATABASE_IP;
// const char *host = "192.168.2.132";
// const char *host = "192.168.2.174"; //green board


#ifdef LAMINATE_ROOM
    // Set your Static IP address
    #ifdef CYCLING
        IPAddress local_ip(200, 10, 2, 214);
        IPAddress gateway(200, 10, 2, 1);
    #else
        IPAddress local_ip(200, 10, 2, 200);
        IPAddress gateway(200, 10, 2, 1);
    #endif
#else
    // Set your Static IP address
    IPAddress local_ip(192, 168, 2, 201);
    IPAddress gateway(192, 168, 2, 1);
#endif

// Set your Gateway IP address
// IPAddress primaryDNS(192, 168, 2, 1);        // optional
// IPAddress secondaryDNS(119, 18, 156, 10);       // optional
String hostName = HOST_NAME;

AsyncWebServer server(80);
TalisRS485Handler talis;
WiFiSetting wifiSetting;
TalisMemory talisMemory;
JsonManager jsonManager;
RMSManager rmsManager;
LedAnimation ledAnimation(8,8, true);
Updater updater[8];
OneButton startButton(32, true, true);

ModbusServerTCPasync MBserver; 

PackedData packedData;
CellData cellData[8];
int cellDataSize = sizeof(cellData) / sizeof(cellData[0]);
RMSInfo rmsInfo;
AlarmParam alarmParam;
HardwareAlarm hardwareAlarm;
CellBalancingCommand cellBalancingCommand;
AddressingCommand addressingCommand;
AlarmCommand alarmCommand;
SleepCommand sleepCommand;
DataCollectionCommand dataCollectionCommand;
DataCollectionCommand testDataCollection;
CellBalancingStatus cellBalancingStatus[8];
CommandStatus commandStatus;
RmsCodeWrite rmsCodeWrite;
RmsRackSnWrite rmsRackSnWrite;
FrameWrite frameWrite;
CMSCodeWrite cmsCodeWrite;
BaseCodeWrite baseCodeWrite;
McuCodeWrite mcuCodeWrite;
SiteLocationWrite siteLocationWrite;
CMSShutDown cmsShutDown;
CMSWakeup cmsWakeup;
LedCommand ledCommand;
CMSRestartCommand cmsRestartCommand;
RMSRestartCommand rmsRestartCommand;
OtaParameter otaParameter;
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

int dataComplete = 0;

// uint16_t msgCount[16];
// int addressListStorage[32];
// Vector<int> addressList(addressListStorage);

uint8_t currentAddrBid;
int8_t isDataNormalList[12];

bool balancingCommand = false;
bool commandCompleted = false;
bool responseCompleted = false;
bool sendCommand = true;
bool isGotCMSInfo = false;
bool lastStateDataCollection = false;
bool lastFrameWrite = false;
bool lastCmsCodeWrite = false;
bool lastBaseCodeWrite = false; 
bool lastMcuCodeWrite = false; 
bool lastSiteLocationWrite = false;
bool lastCellBalancingSball = false;
bool lastCMSShutdown = false;
bool lastCMSWakeup = false;
bool lastIsGotCmsInfo = false;
bool lastLedset = false;
bool isCmsRestartPin = false;
bool cycle = false;
bool isFromSequence = false;
bool addressingByButton = false;
bool manualOverride = false;
bool factoryReset = false;
bool forceBuzzer = false;
bool forceRelay = false;

int isAddressingCompleted = 0;
int commandSequence = 0;
int deviceAddress = 1;
int lastDeviceAddress = 16;
int cmsInfoRetry = 0;
unsigned long lastTime = 0;
unsigned long lastReceivedSerialData = 0;
unsigned long lastReconnectMillis = 0;
int reconnectInterval = 5000;
String commandString;
String responseString;
String serverName = SERVER_NAME;
// String serverName = "http://desktop-gu3m4fp.local/mydatabase/";
String rmsCode = "RMS-32-NA";
String rackSn = "RACK-32-NA";

uint8_t commandOrder[6] = {
    TalisRS485::RequestType::CMSINFO,
    TalisRS485::RequestType::VCELL,
    TalisRS485::RequestType::TEMP,
    TalisRS485::RequestType::VPACK,
    TalisRS485::RequestType::CMSSTATUS,
    TalisRS485::RequestType::CMSREADBALANCINGSTATUS
};

void reInitCellData()
{
    for (size_t i = 0; i < 8; i++)
    {
        cellData[i].frameName = "FRAME-32-NA";
        cellData[i].cmsCodeName = "CMS-32-NA";
        cellData[i].baseCodeName = "BASE-32-NA";
        cellData[i].mcuCodeName = "MCU-32-NA";
        cellData[i].siteLocation = "SITE-32-NA";
        cellData[i].bid = 0;
        cellData[i].msgCount = 0;
        Utilities::fillArray<int>(cellData[i].vcell, 45, -1);
        Utilities::fillArray<int32_t>(cellData[i].temp, 9, -1);
        Utilities::fillArray<int32_t>(cellData[i].pack, 3, -1);
        cellData[i].packStatus.bits.status = 0;
        cellData[i].packStatus.bits.door = 0;
    }
}

void declareStruct()
{
    packedData.rackSn = rackSn;
    packedData.p = cellData;
    packedData.size = cellDataSize;
    packedData.rmsInfoPtr = &rmsInfo;
    for (size_t i = 0; i < cellDataSize; i++)
    {
        cellData[i].frameName = "FRAME-32-NA";
        cellData[i].cmsCodeName = "CMS-32-NA";
        cellData[i].baseCodeName = "BASE-32-NA";
        cellData[i].mcuCodeName = "MCU-32-NA";
        cellData[i].siteLocation = "SITE-32-NA";
        cellData[i].ver = "VER-32-NA";
        cellData[i].chip = "CHIP-32-NA";
        cellData[i].bid = 0;
        cellData[i].msgCount = 0;
        Utilities::fillArray<int>(cellData[i].vcell, 45, -1);
        Utilities::fillArray<int32_t>(cellData[i].temp, 9, -1);
        Utilities::fillArray<int32_t>(cellData[i].pack, 3, -1);
        cellData[i].packStatus.bits.status = 0;
        cellData[i].packStatus.bits.door = 0;
    }
    rmsInfo.rmsCode = rmsCode;
    rmsInfo.rackSn = rackSn;
    rmsInfo.ver = "1.0.0";
    IPAddress defIp(0 ,0, 0, 0);
    // rmsInfo.ip = defIp.toString();
    rmsInfo.ip = wifiSetting.getIp();
    rmsInfo.mac = WiFi.macAddress();
    rmsInfo.deviceTypeName = "RMS";

    // for (size_t i = 0; i < 8; i++)
    // {
    //     addressList.push_back(i+1);
    // }

    commandStatus.addrCommand = 0;
    commandStatus.alarmCommand = 0;
    commandStatus.dataCollectionCommand = 0;
    commandStatus.sleepCommand = 0;

}

int readVcell(const String &input)
{
    int bid = -1;
    int status = -1;
    int cell[45];
    int startIndex; // bid start from 1, array index start from 0
    bool isAllDataCaptured = false;
    bool isAllDataNormal = true;
    bool flag = true;
    bool undervoltageFlag = false;
    bool overvoltageFlag = false;
    bool diffVoltageFlag = false;
    bool isValidJsonFormat = true;
    DynamicJsonDocument docBattery(1024);
    DeserializationError error = deserializeJson(docBattery, input);    
    // Serial.println("Read Vcell");
    if (error) 
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return status;
    }
    // serializeJson(docBattery, jsonDoc);
    // Serial.println(jsonDoc);
    JsonObject object = docBattery.as<JsonObject>();
    if (!object.isNull())
    {        
        if (docBattery.containsKey("BID") && docBattery.containsKey("VCELL"))
        {
            bid = docBattery["BID"];
            startIndex = bid - 1;
            cellData[startIndex].bid = bid;
            // Serial.println("Contains key VCELL");
            JsonArray jsonArray = docBattery["VCELL"].as<JsonArray>();
            // int arrSize = sizeof(docBattery["VCELL"]) / sizeof(docBattery["VCELL"][0]);
            int arrSize = jsonArray.size();
            // Serial.println("Array Size = " + String(arrSize));
            if (arrSize >= 45)
            {
                Serial.println("Vcell Reading Address : " + String(bid));                
                for (int i = 0; i < 45; i++)
                {
                    cell[i] = docBattery["VCELL"][i];
                    cellData[startIndex].vcell[i] = cell[i];
                    Serial.println("Vcell " + String(i+1) + " = " + String(cell[i]));
                }
                isAllDataCaptured = true;
                // msgCount[startIndex]++;
                // cellData[startIndex].msgCount = msgCount[startIndex];
            }
        }
        else
        {
            isValidJsonFormat = false;
            return status;
        }
    }
    else
    {
        return status;
    }
    

    if (isAllDataCaptured)
    {
        int maxVcell = cell[0];
        int minVcell = cell[0];
        for (int c : cell)
        {
            if (c <= 200) //ignore the unconnected cell
            {
                isAllDataNormal = true;
            }
            else  
            {
                if (maxVcell < c) 
                {
                    maxVcell = c;
                }
                
                if (minVcell > c) {
                    minVcell = c;
                }

                int diff = maxVcell - minVcell;

                Serial.println("Max Vcell : " + String(maxVcell));
                Serial.println("Min Vcell : " + String(minVcell));
                Serial.println("Diff : " + String(diff));

                if (c >= alarmParam.vcell_undervoltage && c <= alarmParam.vcell_overvoltage) 
                {    
                    if (cellData[startIndex].packStatus.bits.cellUndervoltage)
                    {
                        if (c < alarmParam.vcell_reconnect)
                        {
                            isAllDataNormal = false;
                            flag = false;
                            undervoltageFlag = true;
                        }
                    }
                }
                else
                {
                    // Serial.println("Vcell min = " + String(alarmParam.vcell_undervoltage));
                    // Serial.println("Vcell max = " + String(alarmParam.vcell_overvoltage));
                    if (c < alarmParam.vcell_undervoltage) 
                    {
                        cellData[startIndex].packStatus.bits.cellUndervoltage = 1;
                        isAllDataNormal = false;
                        flag = false;    
                        undervoltageFlag = true;
                    }
                    
                    if (c > alarmParam.vcell_overvoltage)
                    {
                        cellData[startIndex].packStatus.bits.cellOvervoltage = 1;
                        overvoltageFlag = true;
                    }
                    // Serial.println("Abnormal Cell Voltage = " + String(c));
                    // break;
                }

                if (cellData[startIndex].packStatus.bits.cellDiffAlarm) 
                {
                    if (diff > alarmParam.vcell_diff_reconnect) 
                    {
                        isAllDataNormal = false;
                        flag = false;
                        diffVoltageFlag = true;
                        // break;
                    }
                }
                else
                {
                    if (diff > alarmParam.vcell_diff) 
                    {
                        cellData[startIndex].packStatus.bits.cellDiffAlarm = 1;
                        isAllDataNormal = false;
                        flag = false;
                        diffVoltageFlag = true;
                        // break;
                    }
                }
                
            }

        }

        cellData[startIndex].packStatus.bits.cellDiffAlarm = diffVoltageFlag;
        cellData[startIndex].packStatus.bits.cellUndervoltage = undervoltageFlag;
        cellData[startIndex].packStatus.bits.cellOvervoltage = overvoltageFlag;
        isAllDataNormal = isAllDataNormal & flag;

        if (!isAllDataNormal)
        {
            ESP_LOGI(TAG,"Vcell Data Abnormal");          
        }
        else 
        {
            ESP_LOGI(TAG,"Vcell Data Normal");
        }

        updater[startIndex].updateVcell(isAllDataNormal);
        status = bid;
    }
    else
    {
        if(isValidJsonFormat)
        {
            ESP_LOGI(TAG,"Cannot Capture Vcell Data");
        }
    }
    return status;
}

int readTemp(const String &input)
{
    int bid = 0;
    int startIndex = 0;
    int status = -1;
    int32_t temp[9];
    bool isAllDataCaptured = false;
    bool isAllDataNormal = true;
    bool flag = true;
    bool undertemperatureFlag = false;
    bool overtemperatureFlag = false;
    bool isValidJsonFormat = true;
    DynamicJsonDocument docBattery(1024);
    DeserializationError error = deserializeJson(docBattery, input);
    // Serial.println("Read Temp");
    if (error) 
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return status;
    }
    // Serial.println(jsonDoc);
    JsonObject object = docBattery.as<JsonObject>();
    if (!object.isNull())
    {
        if (docBattery.containsKey("BID") && docBattery.containsKey("TEMP"))
        {
            bid = docBattery["BID"];
            startIndex = bid - 1;
            cellData[startIndex].bid = bid;
            // Serial.println("Contain TEMP key value");
            JsonArray jsonArray = docBattery["TEMP"].as<JsonArray>();
            int arrSize = jsonArray.size();
            if (arrSize >= 9)
            {
                Serial.println("Temperature Reading Address : " + String(bid));
                for (int i = 0; i < 9; i++)
                {
                    float storage = docBattery["TEMP"][i];
                    temp[i] = static_cast<int32_t> (storage * 1000);
                    docBattery["TEMP"][i] = temp[i];
                    cellData[startIndex].temp[i] = temp[i];
                    Serial.println("Temperature " + String(i+1) + " = " + String(temp[i]));
                }
                isAllDataCaptured = true;
                // msgCount[startIndex]++;
                // cellData[startIndex].msgCount = msgCount[startIndex];
            }
            
        }
        else
        {
            isValidJsonFormat = false;
            return status;
        }
            
    }
    else
    {
        return status;
    }
    
    if (isAllDataCaptured)
    {
        int maxTemp = temp[0];
        int minTemp = temp[0];
        for (int32_t temperature : temp)
        {
            if (maxTemp < temperature)
            {
                maxTemp = temperature;
            }

            if (minTemp > temperature)
            {
                minTemp = temperature;
            }
            
            if (maxTemp > alarmParam.temp_max)
            {
                cellData[startIndex].packStatus.bits.overtemperature = 1;
                overtemperatureFlag = true;
            }

            if (minTemp < alarmParam.temp_min)
            {
                cellData[startIndex].packStatus.bits.undertemperature = 1;
                undertemperatureFlag = true;
            }

            if (temperature > alarmParam.temp_max || temperature < alarmParam.temp_min)
            {
                ESP_LOGI(TAG,"Abnormal temperature : %d\n", temperature);
                isAllDataNormal = false;
                flag = false;
                // break;
            }
            else
            {
                isAllDataNormal = true;
            }
        }

        cellData[startIndex].packStatus.bits.undertemperature = undertemperatureFlag;
        cellData[startIndex].packStatus.bits.overtemperature = overtemperatureFlag;

        isAllDataNormal = isAllDataNormal & flag;
        if (isAllDataNormal)
        {
            ESP_LOGI(TAG, "Data Temperature Normal");
        }
        else
        {
            ESP_LOGI(TAG, "Data Temperature Abnormal");
        }
        updater[startIndex].updateTemp(isAllDataNormal);
        status = bid;
    }
    else
    {
        if (isValidJsonFormat)
        {
            ESP_LOGI(TAG, "Cannot Capture Temperature Data");
        }
    }
    return status;
}


int readVpack(const String &input)
{
    int bid = 0;
    int startIndex = 0;
    int status = -1;
    int32_t vpack[4];
    bool isAllDataCaptured = false;
    bool isAllDataNormal = false;
    bool isValidJsonFormat = true;
    DynamicJsonDocument docBattery(1024);
    DeserializationError error = deserializeJson(docBattery, input);
    // Serial.println("Read Vpack");
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
        if (docBattery.containsKey("BID") && docBattery.containsKey("VPACK"))
        {
            bid = docBattery["BID"];
            startIndex = bid - 1;

            // Serial.println("Contain VPACK key Value");
            JsonArray jsonArray = docBattery["VPACK"].as<JsonArray>();
            int arrSize = jsonArray.size();
            // Serial.println("Arr Size = " + String(arrSize));
            if (arrSize >= 4)
            {
                Serial.println("Vpack Reading Address : " + String(bid));
                for (int i = 0; i < 4; i++)
                {
                    vpack[i] = docBattery["VPACK"][i];
                    if (i != 0) // index 0 is for total vpack
                    {
                        cellData[startIndex].pack[i - 1] = vpack[i];
                        Serial.println("Vpack " + String(i) + " = " + String(vpack[i]));
                    }
                    else
                    {
                        Serial.println("Vpack Total = " + String(vpack[i]));
                    }
                }
                isAllDataCaptured = true;
                // msgCount[startIndex]++;
                // cellData[startIndex].msgCount = msgCount[startIndex];
            }
            
        }
        else
        {
            isValidJsonFormat = false;
            return status;
        }
    }
    else
    {
        return status;
    }
    

    if (isAllDataCaptured)
    {       
        for (int32_t vpackValue : vpack)
        {
            if (vpackValue > 0)
            {
                isAllDataNormal = true;
            }
            else
            {
                isAllDataNormal = false;
                break;
            }
        }

        if(isAllDataNormal)
        {
            ESP_LOGI(TAG, "Vpack Data Normal");
        }
        else
        {
            ESP_LOGI(TAG, "Vpack Data Abnormal");
        }
        updater[startIndex].updateVpack(isAllDataNormal);
        status = bid;
    }
    else
    {
        if (isValidJsonFormat)
        {
            ESP_LOGI(TAG, "Cannot Capture Vpack Data");
        }
    }
    return status;
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

int readCMSInfo(const String &input)
{
    int bid = 0;
    int startIndex = 0;
    int status = -1;
    bool isAllDataCaptured = false;
    bool isAllDataNormal = false;
    bool isValidJsonFormat = true;
    DynamicJsonDocument docBattery(1024);
    DeserializationError error = deserializeJson(docBattery, input);
    // Serial.println("Read CMS Info");
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
        // Serial.println("Processing RMS info Request");
        if(docBattery.containsKey("bid"))
        {
            bid = docBattery["bid"];
            startIndex = bid - 1;
        }
        else
        {
            return status;
        }

        if (docBattery.containsKey("cms_code"))
        {
            cellData[startIndex].frameName = docBattery["frame_name"].as<String>();
            cellData[startIndex].bid = bid;
            cellData[startIndex].cmsCodeName = docBattery["cms_code"].as<String>();
            cellData[startIndex].baseCodeName = docBattery["base_code"].as<String>();
            cellData[startIndex].mcuCodeName = docBattery["mcu_code"].as<String>();
            cellData[startIndex].siteLocation = docBattery["site_location"].as<String>();
            cellData[startIndex].ver = docBattery["ver"].as<String>();
            cellData[startIndex].chip = docBattery["chip"].as<String>();
            status = bid;
        }
        else
        {
            return status;
        }      
    }
    return status;
}

int readCMSBalancingResponse(const String &input)
{
    int bid = 0;
    int status = -1;
    int startIndex = 0;
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, input);
    // Serial.println("Read CMS Balancing Response");
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return status;
    }

    if (!(doc.containsKey("RBAL1.1") && doc.containsKey("RBAL2.1") && doc.containsKey("RBAL3.1")))
    {
        return status;
    }

    if (doc.containsKey("BID"))
    {
        bid = doc["BID"];
        startIndex = bid - 1;
        cellBalancingStatus[startIndex].bid = bid;
    }
    else
    {
        return status;
    }

    int rbal[3];
    rbal[0] = doc["RBAL1.1"];
    rbal[1] = doc["RBAL1.2"];
    rbal[2] = doc["RBAL1.3"];
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i] = Utilities::getBit(i, rbal[0]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+5] = Utilities::getBit(i, rbal[1]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+10] = Utilities::getBit(i, rbal[2]);
    }

    rbal[0] = doc["RBAL2.1"];
    rbal[1] = doc["RBAL2.2"];
    rbal[2] = doc["RBAL2.3"];
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+15] = Utilities::getBit(i, rbal[0]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+20] = Utilities::getBit(i, rbal[1]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+25] = Utilities::getBit(i, rbal[2]);
    }

    rbal[0] = doc["RBAL3.1"];
    rbal[1] = doc["RBAL3.2"];
    rbal[2] = doc["RBAL3.3"];
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+30] = Utilities::getBit(i, rbal[0]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+35] = Utilities::getBit(i, rbal[1]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+40] = Utilities::getBit(i, rbal[2]);
    }
    status = bid;
    Serial.println("Balancing Read Success");
    return status;
}

int readCMSBQStatusResponse(const String &input)
{
    int status = -1;
    int door = -1;
    int bid = 0;
    int startIndex = 0;
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, input);
    // Serial.println("Read CMS BQ Status");
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return status;
    }

    if(!doc.containsKey("WAKE_STATUS"))
    {
        // Serial.println("Does not contain WAKE_STATUS");
        return status;
    }

    if(!doc.containsKey("DOOR_STATUS"))
    {
        // Serial.println("Does not contain WAKE_STATUS");
        return status;
    }

    if(!doc.containsKey("BID"))
    {
        // Serial.println("Does not contain BID");
        return status;
    }

    // Serial.println("GET WAKE STATUS");
    bid = doc["BID"];
    startIndex = bid - 1;
    status = doc["WAKE_STATUS"];
    door = doc["DOOR_STATUS"];
    // Serial.println("WAKE STATUS = " + String(status));
    cellData[startIndex].packStatus.bits.status = status;
    cellData[startIndex].packStatus.bits.door = door;
    Serial.println("Status Read Success");
    updater[startIndex].updateStatus();
    status = bid;
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

void addressing(bool isFromBottom)
{
    addressList.clear();
    for (size_t i = 0; i < numOfShiftRegister; i++)
    {
        ESP_LOGI(TAG, "Increment : %d\n", i);
        uint8_t x;
        if(isFromBottom)
        {
            x = (8 * (numOfShiftRegister - i)) - 1; // Q7 is connected to addressing pin of CMS
        }
        else
        {
            x = (numOfShiftRegister * i) + 7;
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

void handleSketchDownload(const OtaParameter &otaParameter) {
    const char* SERVER = "www.my-hostname.it";  // Set your correct hostname
    const unsigned short SERVER_PORT = 443;     // Commonly 80 (HTTP) | 443 (HTTPS)
    const char* PATH = "/update-v%d.bin";       // Set the URI to the .bin firmware

    String url = otaParameter.server + otaParameter.path;

    WiFiClient wifiClient;
    //   HTTPClient client(wifiClient, SERVER, SERVER_PORT);  // HTTP
    // ESPhttpUpdate.update(wifiClient, otaParameter.port, url, "1");
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
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    rmsInfo.ip = WiFi.localIP().toString();
    Serial.print("Subnet Mask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("Gateway IP: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("DNS 1: ");
    Serial.println(WiFi.dnsIP(0));
    Serial.print("DNS 2: ");
    Serial.println(WiFi.dnsIP(1));
    Serial.print("Hostname: ");
    Serial.println(WiFi.getHostname());
    digitalWrite(internalLed, HIGH);
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
    Serial.println("Wifi Connected");
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
    digitalWrite(internalLed, LOW);
    Serial.println("Disconnected from WiFi access point");
    Serial.print("WiFi lost connection. Reason: ");
    Serial.println(info.wifi_sta_disconnected.reason);
    Serial.println("Trying to Reconnect");
    #ifndef DEBUG
        WiFi.begin(ssid, password);
    #else
        WiFi.begin(ssid, password);
    #endif
    
}

void resetUpdater()
{
    for (size_t i = 0; i < 8; i++)
    {
        updater[i].resetUpdater();
    }
    
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
    cmsRestartCommand.bid = 255;
    cmsRestartCommand.restart = 1;
    timerStop(myTimer);
    timerWrite(myTimer, 0);
}

void buttonDoubleClicked()
{
    dataCollectionCommand.exec = 0;
    alarmCommand.buzzer = 0;
    cmsRestartCommand.bid = 255;
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
  xQueueSend(rs485ReceiverQueue, &msg, 0);
}

void onErrorCb(TalisRS485::Error errorCode, uint32_t token)
{
  TalisRS485RxMessage msg;
  msg.token = token;
  msg.error = errorCode;
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
    ESP_LOGV(TAG, "found \"BID_STATUS\"\n");
    TalisRS485TxMessage txMsg;
    txMsg.token = 2000;
    String output;
    StaticJsonDocument<16> doc;
    doc["BID_ADDRESS"] = currentAddrBid;
    serializeJson(doc, output);
    output += '\n';
    txMsg.dataLength = output.length();
    txMsg.requestCode = TalisRS485::RequestType::ADDRESS;
    txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());
    Serial.print(output);
    // ESP_LOGV(TAG, "Serialize json : \n%s\n", output);
    talis.send(txMsg);
  }

  if (doc.containsKey("RESPONSE"))
  {
    ESP_LOGV(TAG, "Addr bid : %d", currentAddrBid);
    ESP_LOGV(TAG, "Increment bid");
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
            ESP_LOGI(TAG, "Serial incoming");
            ESP_LOGI(TAG, "Message token : %d\n", msgBuffer.token);
            if (msgBuffer.token >= 1000 && msgBuffer.token < 2000)
            {
                ESP_LOGI(TAG, "command from user");
            }
            
            if (msgBuffer.dataLength < 0)
            {
                ESP_LOGI(TAG, "Error on received");
                switch (msgBuffer.error)
                {
                    case TalisRS485::Error::NO_TERMINATE_CHARACTER :
                        ESP_LOGI(TAG, "No terminate character found");
                    break;
                    case TalisRS485::Error::TIMEOUT :
                        ESP_LOGI(TAG, "Timeout");
                    break;
                    case TalisRS485::Error::BUFFER_OVF :
                        ESP_LOGI(TAG, "Buffer Overflow");
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
                switch (msgBuffer.token)
                {
                case TalisRS485::RequestType::CMSINFO :
                    ESP_LOGI(TAG, "reading cms info");
                    readCMSInfo(String(c));
                    break;
                case TalisRS485::RequestType::VCELL :
                    ESP_LOGI(TAG, "reading vcell");
                    bid = readVcell(String(c));
                    xQueueSend(idUpdate, &bid, 0);
                    break;
                case TalisRS485::RequestType::TEMP :
                    ESP_LOGI(TAG, "reading temperature");
                    bid = readTemp(String(c));
                    xQueueSend(idUpdate, &bid, 0);
                    break;
                case TalisRS485::RequestType::VPACK :
                    ESP_LOGI(TAG, "reading vpack");
                    bid = readVpack(String(c));
                    break;
                case TalisRS485::RequestType::CMSSTATUS :
                    ESP_LOGI(TAG, "reading status");
                    bid = readCMSBQStatusResponse(String(c));
                    xQueueSend(idUpdate, &bid, 0);
                    break;
                case TalisRS485::RequestType::CMSREADBALANCINGSTATUS :
                    ESP_LOGI(TAG, "reading balancing status");
                    readCMSBQStatusResponse(String(c));
                    break;
                default:
                    addressingHandler(talis, msgBuffer, currentAddrBid);
                    break;
                }
                
                ESP_LOGI(TAG, "Length : %d\n", msgBuffer.dataLength);
                ESP_LOG_BUFFER_CHAR(TAG, msgBuffer.rxData.data(), msgBuffer.dataLength);
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

void updaterTask(void *pv)
{
    UBaseType_t uxHighWaterMark;
    const char* TAG = "Message Counter Updater Task";
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    while (1)
    {
        int buff;
        if (xQueueReceive(idUpdate, &buff, portMAX_DELAY) == pdTRUE)
        { 
            int index = buff - 1;
            if (buff != 0)
            {
                if (updater[index].isUpdate())
                {
                    isDataNormalList[index] = updater[index].isDataNormal(); 
                    cellData[index].msgCount++;
                    updater[index].resetUpdater();
                }
            }
        }
    }
}


void setup()
{
    // nvs_flash_erase(); // erase the NVS partition and...
    // nvs_flash_init(); // initialize the NVS partition.
    // while(true);
    addressList.reserve(12);
    userCommand.reserve(5);
    xTaskCreate(rs485ReceiverTask, "RS485 Receiver Task", 4096, NULL, 10, &rs485ReceiverTaskHandle);
    xTaskCreate(rs485TransmitterTask, "RS485 Transmitter Task", 4096, NULL, 5, &rs485TransmitterTaskHandle);
    xTaskCreate(updaterTask, "Message Counter Updater Task", 4096, NULL, 5, &counterUpdater);
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

    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
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
    
    if (wifiSetting.getMode() == mode_type::AP)
    {
        Serial.println("AP Connected");
        Serial.print("SSID : ");
        Serial.println(WiFi.softAPSSID());
        Serial.print("IP address: ");
        Serial.println(WiFi.softAPIP().toString());
        Serial.print("Subnet Mask: ");
        Serial.println(WiFi.softAPSubnetMask().toString());
        Serial.print("Hostname: ");
        Serial.println(WiFi.softAPgetHostname());
        digitalWrite(internalLed, HIGH);
    }
    else if (wifiSetting.getMode() == mode_type::AP_STATION)
    {
        Serial.println("AP SSID : ");
        Serial.println(WiFi.softAPSSID());
        Serial.print("IP address: ");
        Serial.println(WiFi.softAPIP().toString());
        Serial.print("Hostname: ");
        Serial.println(WiFi.softAPgetHostname());
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
        hardwareAlarm.enable = 1;
    #endif
    declareStruct();
    Utilities::fillArray<int8_t>(isDataNormalList, 12, -1);
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

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
        request->send(200, "text/plain", "Talis 30 MJ Rack Management System"); });

    server.on("/get-single-cms-data", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        size_t cellDataArrSize = sizeof(cellData) / sizeof(cellData[0]);
        int bid = jsonManager.processSingleCmsDataRequest(request);
        if(bid > 0 && bid <= addressList.size())
        {
            String jsonOutput = jsonManager.buildSingleJsonData(cellData[bid-1]);
            request->send(200, "application/json", jsonOutput);
        }
        else
        {
            request->send(400);
        }
    });

    server.on("/get-cms-data", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        size_t cellDataArrSize = sizeof(cellData) / sizeof(cellData[0]);
        // String jsonOutput = jsonManager.buildJsonData(request, cellData, cellDataArrSize);
        packedData.rackSn = rackSn;
        String jsonOutput = jsonManager.buildJsonData(request, packedData, cellDataArrSize);
        request->send(200, "application/json", jsonOutput); });

    server.on("/get-data", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        String buffer;
        packedData.rackSn = rackSn;
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
        // rmsInfo.ip = WiFi.localIP().toString();
        String jsonOutput = jsonManager.buildJsonRMSInfo(rmsInfo);
        request->send(200, "application/json", jsonOutput); });

    server.on("/get-device-cms-info", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        size_t cmsInfoArrSize = sizeof(cellData) / sizeof(cellData[0]);
        // Serial.println(cmsInfoArrSize);
        String jsonOutput = jsonManager.buildJsonCMSInfo(cellData, cmsInfoArrSize);
        Serial.println(jsonOutput);
        request->send(200, "application/json", jsonOutput); });

    server.on("/get-balancing-status", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        size_t arrSize = sizeof(cellBalancingStatus) / sizeof(cellBalancingStatus[0]);
        String jsonOutput = jsonManager.buildJsonBalancingStatus(cellBalancingStatus, arrSize);
        request->send(200, "application/json", jsonOutput); });

    server.on("/get-alarm-parameter", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        String jsonOutput = jsonManager.buildJsonAlarmParameter(alarmParam);
        request->send(200, "application/json", jsonOutput); });

    server.on("/get-command-status", HTTP_GET, [](AsyncWebServerRequest *request)
    {
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
        addressingStatus.status = isAddressingCompleted;
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

        // preferences.begin("dev_params", 1);
        // alm.vcell_diff = preferences.getUShort("cdiff");
        // alm.vcell_diff_reconnect = preferences.getUShort("cdiff_r");
        // alm.vcell_overvoltage = preferences.getUShort("coverv");
        // alm.vcell_undervoltage = preferences.getUShort("cunderv");
        // alm.vcell_reconnect = preferences.getUShort("cunderv_r");
        // alm.temp_max = preferences.getInt("covert");
        // alm.temp_min = preferences.getInt("cundert");
        // preferences.end();
        request->send(200, "application/json", jsonManager.getUserAlarmSetting(alm)); });

    server.on("/post-test", HTTP_POST, [](AsyncWebServerRequest *request)
    {
        Serial.println("post-test");
        request->send(200, "text/plain", "Test Post"); });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404);
    });

    AsyncCallbackJsonWebHandler *setBalancingHandler = new AsyncCallbackJsonWebHandler("/set-balancing", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonBalancingCommandParser(input.c_str(), cellBalancingCommand);
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
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonAddressingCommandParser(input.c_str());
        isAddressing = status;
        addressingCommand.exec = status;
        commandStatus.addrCommand = status;
        isAddressingCompleted = 0;
        response.replace(":status:", String(status));
        request->send(200, "application/json", response); });

    AsyncCallbackJsonWebHandler *setAlarmHandler = new AsyncCallbackJsonWebHandler("/set-alarm", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonAlarmCommandParser(input.c_str(), alarmCommand);
        response.replace(":status:", String(status));
        request->send(200, "application/json", response); });

    AsyncCallbackJsonWebHandler *setDataCollectionHandler = new AsyncCallbackJsonWebHandler("/set-data-collection", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonDataCollectionCommandParser(input.c_str());
        dataCollectionCommand.exec = status;
        commandStatus.dataCollectionCommand = status;
        response.replace(":status:", String(status));
        request->send(200, "application/json", response); });
    
    AsyncCallbackJsonWebHandler *setLedHandler = new AsyncCallbackJsonWebHandler("/set-led", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonLedParser(input.c_str(), ledCommand);
        response.replace(":status:", String(status));
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
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonAlarmParameterParser(input.c_str(), alarmParam);

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

            // preferences.putUShort("cdiff", alarmParam.vcell_diff);
            // preferences.putUShort("cdiff_r", alarmParam.vcell_diff_reconnect);
            // preferences.putUShort("coverv", alarmParam.vcell_overvoltage);
            // preferences.putUShort("cunderv", alarmParam.vcell_undervoltage);
            // preferences.putUShort("cunderv_r", alarmParam.vcell_reconnect);
            // preferences.putInt("covert", alarmParam.temp_max);
            // preferences.putInt("cundert", alarmParam.temp_min);
            // preferences.putChar("p_flag", 2); // parameter flag, 0 to initialize key, 1 to load from default, 2 to load from user

            response.replace(":status:", String(status));
            request->send(200, "application/json", response);    
        }
        else {
            request->send(400);
        } });
        

    AsyncCallbackJsonWebHandler *setHardwareAlarmHandler = new AsyncCallbackJsonWebHandler("/set-hardware-alarm", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonHardwareAlarmEnableParser(input.c_str(), hardwareAlarm);
        response.replace(":status:", String(status));
        request->send(200, "application/json", response); });

    AsyncCallbackJsonWebHandler *setSleepHandler = new AsyncCallbackJsonWebHandler("/set-sleep", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonCMSShutdownParser(input.c_str(), cmsShutDown);
        cmsShutDown.shutdown = status;
        response.replace(":status:", String(status));
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
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonCMSWakeupParser(input.c_str(), cmsWakeup);
        cmsWakeup.wakeup = status;
        response.replace(":status:", String(status));
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
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonCMSRestartParser(input.c_str(), cmsRestartCommand);
        cmsRestartCommand.restart = status;
        addressList.clear();
        response.replace(":status:", String(status));
        if (status >= 0)
        {
            if (status > 0)
            {
                TalisRS485TxMessage txMsg;
                txMsg.id = cmsRestartCommand.bid;
                TalisRS485Message::createCMSResetRequest(txMsg);
                txMsg.token = TalisRS485::RequestType::RESTART;
                userCommand.push_back(txMsg);
            }
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        });
    
    AsyncCallbackJsonWebHandler *restartCMSViaPinHandler = new AsyncCallbackJsonWebHandler("/restart-cms-via-pin", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonRMSRestartParser(input.c_str());
        isCmsRestartPin = true;
        status = isCmsRestartPin;
        addressList.clear();
        response.replace(":status:", String(status));
        request->send(200, "application/json", response); });

    AsyncCallbackJsonWebHandler *restartRMSHandler = new AsyncCallbackJsonWebHandler("/restart", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonRMSRestartParser(input.c_str());
        rmsRestartCommand.restart = status;
        response.replace(":status:", String(status));
        request->send(200, "application/json", response); });

    AsyncCallbackJsonWebHandler *setRmsCodeHandler = new AsyncCallbackJsonWebHandler("/set-rms-code", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = 0;
        jsonManager.jsonRmsCodeParser(input.c_str(), rmsCodeWrite);
        if (rmsCodeWrite.write)
        {
            status = talisMemory.setRmsName(rmsCodeWrite.rmsCode.c_str());
            if (status)
            {
                rmsCode = talisMemory.getRmsName();
            }
            // status = writeToEeprom(EEPROM_RMS_CODE_ADDRESS, EEPROM_RMS_ADDRESS_CONFIGURED_FLAG, rmsCodeWrite.rmsCode, rmsCode);
            // if(status)
            // {
            //     rmsInfo.rmsCode = rmsCodeWrite.rmsCode;
            // }
        }
        rmsCodeWrite.write = 0;
        response.replace(":status:", String(status));
        request->send(200, "application/json", response); });

    AsyncCallbackJsonWebHandler *setRackSnHandler = new AsyncCallbackJsonWebHandler("/set-rack-sn", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = 0;
        jsonManager.jsonRmsRackSnParser(input.c_str(), rmsRackSnWrite);
        if (rmsRackSnWrite.write)
        {
            
            status = talisMemory.setRackSn(rmsRackSnWrite.rackSn.c_str());
            if (status)
            {
                rackSn = talisMemory.getRackSn();
            }
            // status = writeToEeprom(EEPROM_RACK_SN_ADDRESS, EEPROM_RACK_SN_CONFIGURED_FLAG, rmsRackSnWrite.rackSn, rackSn);
            // if(status)
            // {
            //     rmsInfo.rackSn = rmsRackSnWrite.rackSn;
            // }
        }
        rmsRackSnWrite.write = 0;
        response.replace(":status:", String(status));
        request->send(200, "application/json", response); });

    AsyncCallbackJsonWebHandler *setFrameHandler = new AsyncCallbackJsonWebHandler("/set-frame", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonCMSFrameParser(input.c_str(), frameWrite);
        response.replace(":status:", String(status));
        if (status >= 0)
        {
            if (status > 0)
            {
                TalisRS485TxMessage txMsg;
                txMsg.token = TalisRS485::RequestType::CMSFRAMEWRITE;
                txMsg.id = frameWrite.bid;
                TalisRS485Message::createCMSFrameWriteIdRequest(txMsg, frameWrite.frameName);
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
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonCMSCodeParser(input.c_str(), cmsCodeWrite);
        TalisRS485TxMessage txMsg;
        txMsg.token = TalisRS485::RequestType::CMSCODEWRITE;
        txMsg.id = cmsCodeWrite.bid;
        TalisRS485Message::createCMSCodeWriteRequest(txMsg, cmsCodeWrite.cmsCode);
        userCommand.push_back(txMsg);
        response.replace(":status:", String(status));
        request->send(200, "application/json", response); });

    AsyncCallbackJsonWebHandler *setBaseCodeHandler = new AsyncCallbackJsonWebHandler("/set-base-code", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonCMSBaseCodeParser(input.c_str(), baseCodeWrite);
        response.replace(":status:", String(status));
        if (status >= 0)
        {
            if (status > 0)
            {
                TalisRS485TxMessage txMsg;
                txMsg.token = TalisRS485::RequestType::CMSBASECODEWRITE;
                txMsg.id = baseCodeWrite.bid;
                TalisRS485Message::createCMSBaseCodeWriteRequest(txMsg, baseCodeWrite.baseCode);
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
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonCMSMcuCodeParser(input.c_str(), mcuCodeWrite);
        response.replace(":status:", String(status));
        if (status >= 0)
        {
            if (status > 0)
            {
                TalisRS485TxMessage txMsg;
                txMsg.token = TalisRS485::RequestType::CMSMCUCODEWRITE;
                txMsg.id = mcuCodeWrite.bid;
                TalisRS485Message::createCMSMcuCodeWriteRequest(txMsg, mcuCodeWrite.mcuCode);
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
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonCMSSiteLocationParser(input.c_str(), siteLocationWrite);
        response.replace(":status:", String(status));
        if (status >= 0)
        {
            if (status > 0)
            {
                TalisRS485TxMessage txMsg;
                txMsg.token = TalisRS485::RequestType::CMSSITELOCATIONWRITE;
                txMsg.id = siteLocationWrite.bid;
                TalisRS485Message::createCMSSiteLocationWriteRequest(txMsg, siteLocationWrite.siteLocation);
                userCommand.push_back(txMsg);
            }
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400, "application/json", response);
        }
        });

    AsyncCallbackJsonWebHandler *setOtaUpdate = new AsyncCallbackJsonWebHandler("/ota-update", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonOtaUpdate(input.c_str(), otaParameter);
        response.replace(":status:", String(status));
        request->send(200, "application/json", response); });

    AsyncCallbackJsonWebHandler *setNetwork = new AsyncCallbackJsonWebHandler("/set-network", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        
        NetworkSetting setting = jsonManager.parseNetworkSetting(json);
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

            // preferences.putString("ssid", setting.ssid);
            // preferences.putString("pass", setting.pass);
            // preferences.putString("ip", setting.ip);
            // preferences.putString("gateway", setting.gateway);
            // preferences.putString("subnet", setting.subnet);
            // preferences.putChar("server", setting.server);
            // preferences.putChar("mode", setting.mode);
            // preferences.putChar("n_flag", 2);
            response.replace(":status:", String(setting.flag));
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400);
        }
    });

    AsyncCallbackJsonWebHandler *setFactoryReset = new AsyncCallbackJsonWebHandler("/set-factory-reset", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        
        int status = jsonManager.parseFactoryReset(json);
        
        if (status == 1 || status == 0)
        {
            if (status)
            {
                factoryReset = 1;
            }
            response.replace(":status:", String(1));
            request->send(200, "application/json", response);
        }
        else
        {
            request->send(400);
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
    server.addHandler(setOtaUpdate);
    server.addHandler(setNetwork);
    server.addHandler(setFactoryReset);
    // server.addHandler(restartCMSViaPinHandler);

    MBserver.registerWorker(1, READ_COIL, &FC01);      // FC=01 for serverID=1
    MBserver.registerWorker(1, READ_HOLD_REGISTER, &FC03);      // FC=03 for serverID=1
    MBserver.registerWorker(1, READ_INPUT_REGISTER, &FC04);     // FC=04 for serverID=1
    MBserver.registerWorker(1, WRITE_COIL, &FC05);     // FC=05 for serverID=1
    MBserver.registerWorker(1, WRITE_HOLD_REGISTER, &FC06);      // FC=06 for serverID=1
    MBserver.registerWorker(1, WRITE_MULT_REGISTERS, &FC16);    // FC=16 for serverID=1

    // AsyncElegantOTA.begin(&server); // Start ElegantOTA
    server.begin();
    resetUpdater();
    Serial.println("HTTP server started");
    lastReconnectMillis = millis();
    // restartCMSViaPin();
    // delay(100);
    dataCollectionCommand.exec = false;
    // cmsRestartCommand.bid = 255;
    // cmsRestartCommand.restart = 1;
    // for (size_t i = 0; i < 12; i++)
    // {
    //     Serial.println("Data normal list : " + String(isDataNormalList[i]));
    // }
    // delay(2000); //wait for CMS to boot
    MBserver.start(502, 10, 20000);
    systemStatus.bits.ready = 1;
    testDataCollection.exec = 1;
    isAddressing = true;
}

void loop()
{    
    // for (size_t i = 0; i < 8; i++)
    // {
    //     cellData[i].bid = i + 1;
    //     Utilities::fillArrayRandom<int>(cellData[i].vcell, 45, 2800, 3800);
    //     Utilities::fillArrayRandom<int32_t>(cellData[i].temp, 9, 10000, 100000);
    //     Utilities::fillArrayRandom<int32_t>(cellData[i].pack, 3, 32000, 40000);
    // }

    if (wifiSetting.getMode() == mode_type::STATION || wifiSetting.getMode() == mode_type::AP_STATION)
    {
        if ((WiFi.status() != WL_CONNECTED) && (millis() - lastReconnectMillis >= reconnectInterval)) {
            digitalWrite(internalLed, LOW);
            Serial.println("Reconnecting to WiFi...");
            WiFi.disconnect();
            WiFi.reconnect();
            lastReconnectMillis = millis();
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
        Serial.println("Factory Reset");
        delay(500);
        ESP.restart();
    }

    // digitalWrite(relay[0], !alarmCommand.powerRelay);
    // digitalWrite(relay[1], !alarmCommand.battRelay);

    // for(int i = 0; i < addressList.size(); i++)
    // {
    //     int isUpdate = 0;
    //     isUpdate = updater[addressList.at(i) - 1].isUpdate();
    //     if(isUpdate)
    //     {            
    //         int isDataNormal = updater[addressList.at(i) - 1].isDataNormal();
    //         isDataNormalList[addressList.at(i) - 1] = isDataNormal;
    //         Serial.println("Bid : " + String(addressList.at(i) - 1));
    //         Serial.println("Normal : "  + String(isDataNormal));
    //         Serial.println("Data is Complete.. Pushing to Database");
    //         cellData[i].msgCount++;
    //         packedData.rackSn = rackSn;
    //         // systemStatus.val = systemStatus.val | cellData[i].packStatus.val;
    //         updater[addressList.at(i) - 1].resetUpdater();    
    //     }
    // }

    // if(dataCollectionCommand.exec)
    if (testDataCollection.exec)
    {
        if(hardwareAlarm.enable)
        {
            // bool error = true;
            bool isDataNormalListUpdated = false;
            int8_t tempData;
            bool buzzerState = 0;
            for(int i = 0; i < addressList.size(); i++)
            {
                // Serial.println("Evaluate data normal");
                // Serial.println("Address List : " + String(addressList.size()));
                // Serial.println("Address List Content :" + String(addressList.at(i)));
                tempData = isDataNormalList[addressList.at(i)-1];
                // Serial.println("temp data " + String(i) + " : " + String(tempData));
                if (tempData < 0)
                {
                    break;
                }
                else
                {                    
                    isDataNormalListUpdated = true;
                    if (tempData > 0) //data normal no alarm
                    {
                        buzzerState = 0;
                    }
                    else //data abnormal alarm
                    {
                        buzzerState = 1;
                        break;
                    }
                }
            }

            bool cellDiffAlm = false;
            bool cellOvervoltage = false;
            bool cellUndervoltage = false;
            bool overtemperature = false;
            bool undertemperature = false;

            for(int i = 0; i < addressList.size(); i++)
            {
                if(cellData[addressList.at(i) - 1].packStatus.bits.cellDiffAlarm)
                {
                    cellDiffAlm = 1; 
                    break;
                }  
            }
            for(int i = 0; i < addressList.size(); i++)
            {
                if(cellData[addressList.at(i) - 1].packStatus.bits.cellOvervoltage)
                {
                    cellOvervoltage = 1; 
                    break;
                }  
            }
            for(int i = 0; i < addressList.size(); i++)
            {
                if(cellData[addressList.at(i) - 1].packStatus.bits.cellUndervoltage)
                {
                    cellUndervoltage = 1; 
                    break;
                }  
            }
            for(int i = 0; i < addressList.size(); i++)
            {
                if(cellData[addressList.at(i) - 1].packStatus.bits.overtemperature)
                {
                    overtemperature = 1; 
                    break;
                }  
            }
            for(int i = 0; i < addressList.size(); i++)
            {
                if(cellData[addressList.at(i) - 1].packStatus.bits.undertemperature)
                {
                    undertemperature = 1;
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
    
    if (command > 5)
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
        ESP_LOGI(TAG, "isAddressing");
        if (talis.isTxQueueEmpty() && userCommand.empty())
        {
            TalisRS485TxMessage txMsg;
            txMsg.id = 255;
            txMsg.requestCode = TalisRS485::RequestType::RESTART;
            txMsg.token = 10000;
            TalisRS485Message::createCMSResetRequest(txMsg);
            talis.send(txMsg);
            delay(2000);
            beginAddressing = true;
            ESP_LOGI(TAG, "reset cms");
        }
    }
    // while(1);
    if (!beginAddressing)
    {
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
            if (!addressList.empty())
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
                            LedData ledData = ledAnimation.update();
                            LedColor ledColor[8];

                            for (size_t i = 0; i < 8; i++)
                            {
                                ledColor[i].red = ledData.red[i];
                                ledColor[i].green = ledData.green[i];
                                ledColor[i].blue = ledData.blue[i];
                            }

                            if(ledData.currentGroup >= 0)
                            {
                                txMsg.id = ledData.currentGroup;
                            }
                            TalisRS485Message::createCMSWriteLedRequest(txMsg, ledColor, 8);
                        }
                    break;
                    default:
                    break;
                }
                
                if (!isAddressing)
                {
                    // ESP_LOGV(TAG, "ID : %d\nCommand : %d\n", id, command);
                    TalisRS485::Error error = talis.addRequest(txMsg);
                    if (error == TalisRS485::Error::SUCCESS)
                    {
                        command++;
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
        addressing(true);
        talis.setTimeout(500);
        talis.resume();
        isAddressing = false;
        beginAddressing = false;
        ESP_LOGI(TAG, "Addressing is finished");
        // talis.resume();
    }
    delay(1);
} // void loop end