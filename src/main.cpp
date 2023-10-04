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

int cell[45];
int32_t temp[9];
int32_t vpack[4]; // index 0 is total vpack
int battRelay = 23;
int buzzer = 26;

int internalLed = 2;

int const numOfShiftRegister = 8;
int address = 1; //BID start from 1

int serialData = 12;
int shcp = 14;
int stcp = 13;

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
JsonManager jsonManager;
RMSManager rmsManager;
LedAnimation ledAnimation(8,8, true);
Updater updater[8];
OneButton startButton(32, true, true);

ModbusServerTCPasync MBserver; 

Data data;
CellData cellData[8];
int cellDataSize = sizeof(cellData) / sizeof(cellData[0]);
RMSInfo rmsInfo;
CMSInfo cmsInfo[8];
AlarmParam alarmParam;
HardwareAlarm hardwareAlarm;
CellAlarm cellAlarm[45];
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
uint16_t ssidArr[16];
uint16_t passArr[16];
uint16_t saveNetwork;
uint16_t ipOctet[4] = {192, 168, 2, 141};
uint16_t gatewayOctet[4] = {192, 168, 2, 1};
uint16_t subnetOctet[4] = {255, 255, 255, 0};
uint16_t gServerType = 2;
uint16_t gMode = 2;

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

namespace Network {
    enum Server {
        STATIC = 1,
        DHCP = 2
    };
    enum MODE {
        AP = 1,
        STATION = 2
    };
};

int dataComplete = 0;

uint16_t msgCount[16];
int addressListStorage[12];
Vector<int> addressList(addressListStorage);
int8_t isDataNormalList[12];

bool balancingCommand = false;
bool commandCompleted = false;
bool responseCompleted = false;
bool isFirstRun = true;
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
unsigned long lastProcessResponse = 0;
unsigned long lastReconnectMillis = 0;
int reconnectInterval = 5000;
String commandString;
String responseString;
String serverName = SERVER_NAME;
// String serverName = "http://desktop-gu3m4fp.local/mydatabase/";
String rmsCode = "RMS-32-NA";
String rackSn = "RACK-32-NA";

uint8_t activeMode;



void setDefaultPreference()
{
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String defaultSsid = "ESP32-" + mac;
    Preferences preferences;
    preferences.begin("dev_params");

    /**
     * Store default network setting
    */
    preferences.putString("d_ssid", defaultSsid);
    preferences.putString("d_pass", "esp32-default");
    preferences.putString("d_ip", "192.168.1.100");
    preferences.putString("d_gateway", "192.168.1.1");
    preferences.putString("d_subnet", "255.255.255.0");
    preferences.putChar("d_server", Network::Server::STATIC);
    preferences.putChar("d_mode", Network::MODE::AP);
    preferences.putChar("n_flag", 1); // network flag, 0 to initialize key, 1 to load from default, 2 to load from user

    /**
     * Store default device parameter setting
    */
    preferences.putUShort("d_cdiff", 300);
    preferences.putUShort("d_cdiff_r", 250);
    preferences.putUShort("d_coverv", 3700);
    preferences.putUShort("d_cunderv", 2800);
    preferences.putUShort("d_cunderv_r", 3000);
    preferences.putInt("d_covert", 80000);
    preferences.putInt("d_cundert", 10000);
    preferences.putChar("p_flag", 1); // parameter flag, 0 to initialize key, 1 to load from default, 2 to load from user

    preferences.end();
}

void setUserPreference()
{
    Preferences preferences;
    preferences.begin("dev_params");

    /**
     * Store user network setting
    */
    preferences.putString("ssid", "RnD_Sundaya");
    preferences.putString("pass", "sundaya22");
    preferences.putString("ip", "192.168.2.162");
    preferences.putString("gateway", "192.168.2.1");
    preferences.putString("subnet", "255.255.255.0");
    preferences.putChar("server", Network::Server::DHCP);
    preferences.putChar("mode", Network::MODE::STATION);
    preferences.putChar("n_flag", 1); // network flag, 0 to initialize key, 1 to load from default, 2 to load from user

    /**
     * Store user device parameter setting
    */
    preferences.putUShort("cdiff", 300);
    preferences.putUShort("cdiff_r", 250);
    preferences.putUShort("coverv", 3700);
    preferences.putUShort("cunderv", 2800);
    preferences.putUShort("cunderv_r", 3000);
    preferences.putInt("covert", 80000);
    preferences.putInt("cundert", 10000);
    preferences.putChar("p_flag", 1); // parameter flag, 0 to initialize key, 1 to load from default, 2 to load from user
    
    preferences.end();
}

template <typename T>
void fillArray(T a[], size_t len, T value)
{
    for (size_t i = 0; i < len; i++)
    {
        a[i] = value;
    }
}

template <typename T>
void fillArrayRandom(T a[], size_t len, T min, T max)
{
    for (size_t i = 0; i < len; i++)
    {
        a[i] = random(min, max);
    }
}

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
        fillArray<int>(cellData[i].vcell, 45, -1);
        fillArray<int32_t>(cellData[i].temp, 9, -1);
        fillArray<int32_t>(cellData[i].pack, 3, -1);
        // for (size_t j = 0; j < 45; j++)
        // {
        //     cellData[i].vcell[j] = -1;
        // }
        // for (size_t j = 0; j < 9; j++)
        // {
        //     cellData[i].temp[j] = -1;
        // }
        // for (size_t j = 0; j < 3; j++)
        // {
        //     cellData[i].pack[j] = -1;
        // }
        cellData[i].packStatus.bits.status = 0;
        cellData[i].packStatus.bits.door = 0;
    }
}

void declareStruct()
{
    data.rackSn = rackSn;
    data.p = cellData;
    data.size = cellDataSize;
    for (size_t i = 0; i < cellDataSize; i++)
    {
        cellData[i].frameName = "FRAME-32-NA";
        cellData[i].cmsCodeName = "CMS-32-NA";
        cellData[i].baseCodeName = "BASE-32-NA";
        cellData[i].mcuCodeName = "MCU-32-NA";
        cellData[i].siteLocation = "SITE-32-NA";
        // cellData[i].bid = 0;
        // for (size_t j = 0; j < 45; j++)
        // {
        //     cellData[i].vcell[j] = 0;
        // }
        // for (size_t j = 0; j < 9; j++)
        // {
        //     cellData[i].temp[j] = 0;
        // }
        // for (size_t j = 0; j < 3; j++)
        // {
        //     cellData[i].pack[j] = 0;
        // }
        // cellData[i].status = 0;
        // cellData[i].door = 0;
    }
    rmsInfo.rmsCode = rmsCode;
    rmsInfo.rackSn = rackSn;
    rmsInfo.ver = "1.0.0";
    rmsInfo.ip = WiFi.localIP().toString();
    rmsInfo.mac = WiFi.macAddress();
    rmsInfo.deviceTypeName = "RMS";

    for (size_t i = 0; i < 8; i++)
    {
        cmsInfo[i].bid = 0;
        cmsInfo[i].cmsCodeName = "CMS-32-NA";
        cmsInfo[i].baseCodeName = "BASE-32-NA";
        cmsInfo[i].mcuCodeName = "MCU-32-NA";
        cmsInfo[i].siteLocation = "SITE-32-NA";
        cmsInfo[i].ver = "VER-32-NA";
        cmsInfo[i].chip = "CHIP-32-NA";
    }

    for (size_t i = 0; i < 8; i++)
    {
        addressList.push_back(i+1);
        // cellBalancingStatus[i].bid = i;
        // for (size_t j = 0; j < 45; j++)
        // {
        //     cellBalancingStatus[i].cball[j] = -1;
        // }
    }

    alarmParam.vcell_diff = 300;
    alarmParam.vcell_diff_reconnect = 250;
    alarmParam.vcell_max = 3700;
    alarmParam.vcell_min = 3000;
    alarmParam.vcell_reconnect = 3200;
    alarmParam.temp_max = 80000;
    alarmParam.temp_min = 20000;

    // for (size_t i = 0; i < 45; i++)
    // {
    //     cellAlarm[i].cell_number = i + 1;
    //     cellAlarm[i].alm_status = 0;
    //     cellAlarm[i].alm_code = 0;
    // }
    commandStatus.addrCommand = 0;
    commandStatus.alarmCommand = 0;
    commandStatus.dataCollectionCommand = 0;
    commandStatus.sleepCommand = 0;

}

int writeToEeprom(int dataAddress, int flagAddress, String &newName, String &oldName)
{
    if(newName.length() < 31) // reserved 1 character for null terminator
    {
        if(newName != oldName)
        {
            EEPROM.writeString(dataAddress, newName);
            EEPROM.write(flagAddress, 1);
            EEPROM.commit();
            oldName = newName;
            return 1; //success writing to eeprom
        }
        return 0; //if newName is equal to oldName
    }
    return -1; //failed to write
}

template <typename T>
String arrToStr(T a[], int len)
{
    String b;
    for (size_t i = 0; i < len; i++)
    {
        if (i == (len - 1))
        {
            b += String(a[i]);
        }
        else
        {
            b += String(a[i]) + ",";
        }
    }
    return b;
}

int readAddressingResponse(const String &input)
{
    // Serial.println("Read BID STATUS");
    int status = -1;
    StaticJsonDocument<32> doc;
    DeserializationError error = deserializeJson(doc, input);

    if (error) {
        // Serial.println("Read BID STATUS Error");
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return status;
    }

    if (!doc.containsKey("BID_STATUS")) 
    {
        return status;
    }
    status = doc["BID_STATUS"]; // 1
    return status;
}

int readAddressing(const String &input)
{
    // Serial.println("Read Addressing");
    int status = -1;
    int bid = 0;
    int respon = 0;
    StaticJsonDocument<128> doc;

    DeserializationError error = deserializeJson(doc, input);

    if (error) {
        // Serial.println("Read Addressing Error");
        Serial.println(input);
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return status;
    }

    // JsonObject object = doc.as<JsonObject>();
    // Serial.println("Read Addressing Deserialize No Error");
    if(!doc.isNull())
    {
        if(doc.containsKey("BID") && doc.containsKey("RESPONSE"))
        {
            // Serial.println("Contain BID and RESPONSE");
            bid = doc["BID"];
            int respon = doc["RESPONSE"];
            if (respon > 0)
            {
                addressList.push_back(bid);
                ledAnimation.setLedGroupNumber(addressList.size());
                status = 1;
                size_t num = addressList.size();
                Serial.println("Address number : " + String(num));
            }
        }
        else
        {
            return status;
        }
    }
    else
    {
        return status;
    }
    return status;
}

int readVcell(const String &input)
{
    int bid = -1;
    int status = -1;
    int led;
    int errTimeout;
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

                if (c >= alarmParam.vcell_min && c <= alarmParam.vcell_max) 
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
                    // Serial.println("Vcell min = " + String(alarmParam.vcell_min));
                    // Serial.println("Vcell max = " + String(alarmParam.vcell_max));
                    if (c < alarmParam.vcell_min) 
                    {
                        cellData[startIndex].packStatus.bits.cellUndervoltage = 1;
                        isAllDataNormal = false;
                        flag = false;    
                        undervoltageFlag = true;
                    }
                    
                    if (c > alarmParam.vcell_max)
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
            Serial.println("Vcell Data Abnormal");
            LedCommand cmd;
            int position = bid - 1;
            cmd.bid = bid;
            cmd.ledset = 1;
            cmd.num_of_led = 8;
            cmd.red[position] = 200;
            cmd.green[position] = 0;
            cmd.blue[position] = 0;
            // String output = rmsManager.createJsonLedRequest(cmd);
            // Serial2.print(output);  
            // Serial2.print('\n');          
        }
        else 
        {
            Serial.println("Vcell Data Normal");
            LedCommand cmd;
            int position = bid - 1;
            cmd.bid = bid;
            cmd.ledset = 1;
            cmd.num_of_led = 8;
            cmd.red[position] = 0;
            cmd.green[position] = 200;
            cmd.blue[position] = 0;
            // String output = rmsManager.createJsonLedRequest(cmd);
            // Serial2.print(output);
            // Serial2.print('\n');
        }

        #ifdef AUTO_POST
            /*
            docBattery["frame_name"] = cellData[startIndex].frameName;
            String phpName = "updatecell.php";
            String link = serverName + phpName;
            HTTPClient http;
            http.begin(link);
            http.addHeader("Content-Type", "application/json");
            String httpPostData;
            serializeJson(docBattery, httpPostData);
            int httpResponseCode = http.POST(httpPostData);
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            Serial.println("berhasil");
            http.end();
            */
        #endif
        updater[startIndex].updateVcell(isAllDataNormal);
        status = 1;
    }
    else
    {
        if(isValidJsonFormat)
        {
            Serial.println("Cannot Capture Vcell Data");
            LedCommand cmd;
            int position = bid - 1;
            cmd.bid = bid;
            cmd.ledset = 1;
            cmd.num_of_led = 8;
            cmd.red[position] = 127;
            cmd.green[position] = 127;
            cmd.blue[position] = 0;
            // String output = rmsManager.createJsonLedRequest(cmd);
            // Serial2.print(output);
            // Serial2.print('\n');
        }
    }
    return status;
}

int readTemp(const String &input)
{
    int bid = 0;
    int startIndex = 0;
    int status = -1;
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
        Serial.println(alarmParam.temp_max);
        Serial.println(alarmParam.temp_min);
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
                Serial.println("ABNORMAL");
                Serial.println(temperature);
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
            Serial.println("Data Temperature Normal");
            LedCommand cmd;
            int position = bid - 1;
            cmd.bid = bid;
            cmd.ledset = 1;
            cmd.num_of_led = 8;
            cmd.red[position] = 0;
            cmd.green[position] = 200;
            cmd.blue[position] = 0;
            // String output = rmsManager.createJsonLedRequest(cmd);
            // Serial2.print(output);
            // Serial2.print('\n');
        }
        else
        {
            Serial.println("Data Temperature Abnormal");
            LedCommand cmd;
            int position = bid - 1;
            cmd.bid = bid;
            cmd.ledset = 1;
            cmd.num_of_led = 8;
            cmd.red[position] = 200;
            cmd.green[position] = 0;
            cmd.blue[position] = 0;
            // String output = rmsManager.createJsonLedRequest(cmd);
            // Serial2.print(output);
            // Serial2.print('\n');
        }
        updater[startIndex].updateTemp(isAllDataNormal);
        status = 1;

        #ifdef AUTO_POST
            /*
            docBattery["frame_name"] = cellData[startIndex].frameName;
            String phpName = "updatetemperature.php";
            String link = serverName + phpName;
            HTTPClient http;
            http.begin(link);
            http.addHeader("Content-Type", "application/json");
            String httpPostData;
            serializeJson(docBattery, httpPostData);
            int httpResponseCode = http.POST(httpPostData);
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            Serial.println("berhasil");
            http.end();
            */
        #endif
        
        /*
        String Link;
        HTTPClient http;

        Link = "http://192.168.2.174/rakbatterybiru/src/logic/bms_temp_update.php?temp=" + String(id) + "," + String(t);
        // http.begin(client, Link);
        // http.GET();
        // http.end();
        leds[led] = CRGB(10, 202, 9);
        FastLED.show();
        Serial.println("TEMP MASUK");

        if (emergency == 8)
        {
            Serial2.println("ERROR TONG");
            digitalWrite(buzzer, LOW);
            state = 1;
            alert = 1;
            BACK = 1;
            return;
        }
        */
    }
    else
    {
        if (isValidJsonFormat)
        {
            Serial.println("Cannot Capture Temperature Data");
            LedCommand cmd;
            int position = bid - 1;
            cmd.bid = bid;
            cmd.ledset = 1;
            cmd.num_of_led = 8;
            cmd.red[position] = 127;
            cmd.green[position] = 127;
            cmd.blue[position] = 0;
            // String output = rmsManager.createJsonLedRequest(cmd);
            // Serial2.print(output);
            // Serial2.print('\n');
        }
    }
    return status;
}


int readVpack(const String &input)
{
    int bid = 0;
    int startIndex = 0;
    int status = -1;
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
            Serial.println("Vpack Data Normal");
            LedCommand cmd;
            int position = bid - 1;
            cmd.bid = bid;
            cmd.ledset = 1;
            cmd.num_of_led = 8;
            cmd.red[position] = 0;
            cmd.green[position] = 200;
            cmd.blue[position] = 0;
            // String output = rmsManager.createJsonLedRequest(cmd);
            // Serial2.print(output);
            // Serial2.print('\n');
        }
        else
        {
            Serial.println("Vpack Data Abnormal");
            LedCommand cmd;
            int position = bid - 1;
            cmd.bid = bid;
            cmd.ledset = 1;
            cmd.num_of_led = 8;
            cmd.red[position] = 200;
            cmd.green[position] = 0;
            cmd.blue[position] = 0;
            // String output = rmsManager.createJsonLedRequest(cmd);
            // Serial2.print(output);
            // Serial2.print('\n');
        }
        updater[startIndex].updateVpack(isAllDataNormal);
        status = 1;

        #ifdef AUTO_POST
            /*
            docBattery["frame_name"] = cellData[startIndex].frameName;
            String phpName = "updatevpack.php";
            String link = serverName + phpName;
            HTTPClient http;
            http.begin(link);
            http.addHeader("Content-Type", "application/json");
            String httpPostData;
            serializeJson(docBattery, httpPostData);
            int httpResponseCode = http.POST(httpPostData);
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            Serial.println("berhasil");
            http.end();
            */
        #endif

        /*
        String Link;
        HTTPClient http;

        Link = "http://192.168.2.174/rakbatterybiru/src/logic/bms_vpack_update.php?vpack=" + String(id) + "," + String(t);
        // http.begin(client, Link);
        // http.GET();

        //        String respon = http.getString();
        //        Serial.println(respon);

        // http.end();
        leds[led] = CRGB(10, 202, 9);
        FastLED.show();
        Serial.println("VPACK MASUK");
        */
    }
    else
    {
        if (isValidJsonFormat)
        {
            Serial.println("Cannot Capture Vpack Data");
            LedCommand cmd;
            int position = bid - 1;
            cmd.bid = bid;
            cmd.ledset = 1;
            cmd.num_of_led = 8;
            cmd.red[position] = 127;
            cmd.green[position] = 127;
            cmd.blue[position] = 0;
            // String output = rmsManager.createJsonLedRequest(cmd);
            // Serial2.print(output);
            // Serial2.print('\n');
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
            // Serial.println("Writing Info to Local Storage");
            cmsInfo[startIndex].frameName = docBattery["frame_name"].as<String>();
            cellData[startIndex].frameName = cmsInfo[startIndex].frameName;
            cmsInfo[startIndex].bid = bid;
            cellData[startIndex].bid = cmsInfo[startIndex].bid;
            cmsInfo[startIndex].cmsCodeName = docBattery["cms_code"].as<String>();
            cellData[startIndex].cmsCodeName = cmsInfo[startIndex].cmsCodeName;
            cmsInfo[startIndex].baseCodeName = docBattery["base_code"].as<String>();
            cellData[startIndex].baseCodeName = cmsInfo[startIndex].baseCodeName;
            cmsInfo[startIndex].mcuCodeName = docBattery["mcu_code"].as<String>();
            cellData[startIndex].mcuCodeName = cmsInfo[startIndex].mcuCodeName;
            cmsInfo[startIndex].siteLocation = docBattery["site_location"].as<String>();
            cellData[startIndex].siteLocation = cmsInfo[startIndex].siteLocation;
            cmsInfo[startIndex].ver = docBattery["ver"].as<String>();
            cmsInfo[startIndex].chip = docBattery["chip"].as<String>();
            status = 1;
        }
        else
        {
            return status;
        }
        
        #ifdef AUTO_POST
            String phpName = "createdatabase.php";
            String link = serverName + phpName;
            HTTPClient http;
            http.begin(link);
            http.addHeader("Content-Type", "application/json");
            int httpResponseCode = http.POST(input);
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            http.end();
        #endif
        
        
    }
    return status;
}

int getBit(int pos, int data)
{
  if (pos > 7 & pos < 0)
  {
    return -1;
  }
  int temp = data >> pos;
  temp = temp & 0x01;
  return temp;
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
        cellBalancingStatus[startIndex].cball[i] = getBit(i, rbal[0]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+5] = getBit(i, rbal[1]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+10] = getBit(i, rbal[2]);
    }

    rbal[0] = doc["RBAL2.1"];
    rbal[1] = doc["RBAL2.2"];
    rbal[2] = doc["RBAL2.3"];
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+15] = getBit(i, rbal[0]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+20] = getBit(i, rbal[1]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+25] = getBit(i, rbal[2]);
    }

    rbal[0] = doc["RBAL3.1"];
    rbal[1] = doc["RBAL3.2"];
    rbal[2] = doc["RBAL3.3"];
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+30] = getBit(i, rbal[0]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+35] = getBit(i, rbal[1]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+40] = getBit(i, rbal[2]);
    }
    status = 1;
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
    #ifdef AUTO_POST
        /*
        doc["frame_name"] = cellData[startIndex].frameName;
        String phpName = "updatewakestatus.php";
        String link = serverName + phpName;
        HTTPClient http;
        http.begin(link);
        http.addHeader("Content-Type", "application/json");
        String httpPostData;
        serializeJson(doc, httpPostData);
        int httpResponseCode = http.POST(httpPostData);
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        Serial.println("berhasil");
        http.end();
        */
    #endif
    status = 1;
    return status;
}


int sendVcellRequest(int bid)
{
    int led = bid - 1;
    // Serial.println("Request Vcell Data");
    leds[led] = CRGB(227, 202, 9);
    FastLED.setBrightness(20);
    FastLED.show();
    String output = rmsManager.createJsonVcellDataRequest(bid);
    Serial2.print(output);
    Serial2.print('\n');
    // Serial.println(output);
    leds[led] = CRGB(129, 141, 214);
    FastLED.show();
    // Serial.println("End Of Vcell Request");
    return 1;
}

int sendTempRequest(int bid)
{
    int led = bid - 1;
    leds[led] = CRGB(227, 202, 9);
    FastLED.setBrightness(20);
    FastLED.show();
    String output = rmsManager.createJsonTempDataRequest(bid);
    Serial2.print(output);
    Serial2.print('\n');
    // Serial.println(output);
    leds[led] = CRGB(129, 141, 214);
    FastLED.show();
    return 1;
}

int sendVpackRequest(int bid)
{
    int led = bid - 1;
    // Serial.println("Request Vpack Data");
    leds[led] = CRGB(227, 202, 9);
    FastLED.setBrightness(20);
    FastLED.show();
    String output = rmsManager.createJsonVpackDataRequest(bid);
    Serial2.print(output);
    Serial2.print('\n');
    // Serial.println(output);
    leds[led] = CRGB(129, 141, 214);
    FastLED.show();
    return 1;
}

int sendCMSReadBalancingStatus(int bid)
{
    int led = bid - 1;
    // Serial.println("Request Vpack Data");
    leds[led] = CRGB(227, 202, 9);
    FastLED.setBrightness(20);
    FastLED.show();
    String output = rmsManager.createCMSReadBalancingStatus(bid);
    Serial2.print(output);
    Serial2.print('\n');
    leds[led] = CRGB(129, 141, 214);
    FastLED.show();
    return 1;
}

int sendCMSInfoRequest(int bid)
{
    int led = bid - 1;
    // Serial.println("Request Vpack Data");
    leds[led] = CRGB(227, 202, 9);
    FastLED.setBrightness(20);
    FastLED.show();
    String output = rmsManager.createCMSInfoRequest(bid);
    Serial2.print(output);
    Serial2.print('\n');
    leds[led] = CRGB(129, 141, 214);
    FastLED.show();
    return 1;
}

int sendCMSFrameWriteRequest(FrameWrite frameWrite)
{
    int led = frameWrite.bid - 1;
    // Serial.println("Request Vpack Data");
    leds[led] = CRGB(227, 202, 9);
    FastLED.setBrightness(20);
    FastLED.show();
    String output = rmsManager.createCMSFrameWriteIdRequest(frameWrite.bid, frameWrite.frameName);
    Serial2.print(output);
    Serial2.print('\n');
    leds[led] = CRGB(129, 141, 214);
    FastLED.show();
    return 1;
}

int sendCMSCodeWriteRequest(CMSCodeWrite cmsCodeWrite)
{
    int led = cmsCodeWrite.bid - 1;
    // Serial.println("Request Vpack Data");
    leds[led] = CRGB(227, 202, 9);
    FastLED.setBrightness(20);
    FastLED.show();
    String output = rmsManager.createCMSCodeWriteRequest(cmsCodeWrite.bid, cmsCodeWrite.cmsCode);
    Serial2.print(output);
    Serial2.print('\n');
    leds[led] = CRGB(129, 141, 214);
    FastLED.show();
    return 1;
}

int sendCMSBaseCodeWriteRequest(BaseCodeWrite baseCodeWrite)
{
    int led = baseCodeWrite.bid - 1;
    // Serial.println("Request Vpack Data");
    leds[led] = CRGB(227, 202, 9);
    FastLED.setBrightness(20);
    FastLED.show();
    String output = rmsManager.createCMSBaseCodeWriteRequest(baseCodeWrite.bid, baseCodeWrite.baseCode);
    Serial2.print(output);
    Serial2.print('\n');
    leds[led] = CRGB(129, 141, 214);
    FastLED.show();
    return 1;
}

int sendCMSMcuCodeWriteRequest(McuCodeWrite mcuCodeWrite)
{
    int led = mcuCodeWrite.bid - 1;
    // Serial.println("Request Vpack Data");
    leds[led] = CRGB(227, 202, 9);
    FastLED.setBrightness(20);
    FastLED.show();
    String output = rmsManager.createCMSMcuCodeWriteRequest(mcuCodeWrite.bid, mcuCodeWrite.mcuCode);
    Serial2.print(output);
    Serial2.print('\n');
    leds[led] = CRGB(129, 141, 214);
    FastLED.show();
    return 1;
}

int sendCMSSiteLocationWriteRequest(SiteLocationWrite siteLocationWrite)
{
    int led = siteLocationWrite.bid - 1;
    // Serial.println("Request Vpack Data");
    leds[led] = CRGB(227, 202, 9);
    FastLED.setBrightness(20);
    FastLED.show();
    String output = rmsManager.createCMSSiteLocationWriteRequest(siteLocationWrite.bid, siteLocationWrite.siteLocation);
    Serial2.print(output);
    Serial2.print('\n');
    leds[led] = CRGB(129, 141, 214);
    FastLED.show();
    return 1;
}

int sendBalancingWriteRequest(CellBalancingCommand cellBalancingCommand)
{
    int led = cellBalancingCommand.bid - 1;
    // Serial.println("Request Vpack Data");
    leds[led] = CRGB(227, 202, 9);
    FastLED.setBrightness(20);
    FastLED.show();
    String output = rmsManager.createCMSWriteBalancingRequest(cellBalancingCommand.bid, cellBalancingCommand.cball);
    Serial2.print(output);
    Serial2.print('\n');
    Serial.println(output);
    leds[led] = CRGB(129, 141, 214);
    FastLED.show();
    return 1;
}

int sendCMSStatusRequest(int bid)
{
    String output = rmsManager.createCMSStatusRequest(bid);
    Serial2.print(output);
    Serial2.print('\n');
    return 1;
}

int sendCMSShutDownRequest(CMSShutDown cmsShutDown)
{
    String output = rmsManager.createShutDownRequest(cmsShutDown.bid);
    Serial2.print(output);
    Serial2.print('\n');
    return 1;
}

int sendCMSWakeupRequest(CMSWakeup cmsWakeup)
{
    String output = rmsManager.createWakeupRequest(cmsWakeup.bid);
    Serial2.print(output);
    Serial2.print('\n');
    return 1;
}

int sendCMSRestartRequest(CMSRestartCommand cmsResetCommand)
{
    String output = rmsManager.createCMSResetRequest(cmsResetCommand.bid);
    Serial2.print(output);
    Serial2.print('\n');
    return 1;
}

int sendLedRequest(LedCommand ledCommand)
{
    String output = rmsManager.createJsonLedRequest(ledCommand);
    Serial2.print(output);
    Serial2.print('\n');
    return 1;
}

void performAlarm()
{
    for (int i = 1; i <= 8; i++)
    {
        for (int a = 1; a <= 3; a++)
        {
            // docBattery["BID"] = i;
            // docBattery["SBQ"] = a;
            // DynamicJsonDocument docBattery(768);
            String output = rmsManager.createShutDownRequest(i);
            Serial2.print(output);
            Serial2.print('\n');
            // serializeJson(docBattery, Serial2);
        }
    }
}

void getDeviceStatus(int id)
{

}

void addressingExec()
{
    isAddressingCompleted = 0;
    
    addressList.clear();
    for (int i = 0; i < numOfShiftRegister; i++)
    {
        bool isRetry = 1;
        int timeout = 0;
        uint8_t x = (8 * i) + 7; // 8 is number of shift register output
        int bid = i + 1; // id start from 1
        sr.set(x, HIGH);
        delay(100);
        sr.set(x, LOW);
        delay(400);
        Serial.print("number = ");
        Serial.println(x);
        Serial.print("EHUB Number -> BMS === ");
        Serial.println(bid);
        // Serial.println(sr.get(x));
        StaticJsonDocument<128> doc;
        String output;
        doc["BID_ADDRESS"] = bid;
        doc["SR"] = x;
        serializeJson(doc, output);
        Serial2.print(output);
        Serial2.print('\n');
        Serial.println("===============xxxxxxxxx===========");
        // delay(200);
        // sr.setAllLow();
    }
    isAddressingCompleted = 1;
}

void setShiftRegisterState()
{
    uint8_t pinValues[numOfShiftRegister] = {B01000000}; //default state, Q6 is set to HIGH because restart is active LOW
    sr.setAll(pinValues);
}

void restartCMSViaPin()
{
    uint8_t pinValues[numOfShiftRegister] = {B00000000};
    sr.setAll(pinValues);
    delay(200);
    for (size_t i = 0; i < numOfShiftRegister; i++)
    {
        pinValues[i] = {B01000000}; //set to default state
    }
    sr.setAll(pinValues);
    delay(200);
}

String parseconfig(String url)
{
    String payload;
    WiFiClient client;
    client.connect(host, 80);

    String Link;
    HTTPClient http;

    Link = url;
    http.begin(client, Link);
    int httpCode = http.GET();
    if (httpCode > 0)
    {
        payload = http.getString();
    }
    http.end();
    return payload;
}

void evalCommand(String input)
{
    if (input == "readcell")
    {
        sendVcellRequest(1);
    }
    else if (input == "readtemp")
    {
        sendTempRequest(1);
    }
    else if (input == "readvpack")
    {
        sendVpackRequest(1);
    }
    else if (input == "startdata")
    {
        dataCollectionCommand.exec = true;
    }
    else if (input == "stopdata")
    {
        dataCollectionCommand.exec = false;
    }
    else if (input == "startaddress")
    {
        addressingCommand.exec = true;
    }
    else if (input == "restartcms")
    {
        Serial.println("Restarting CMS");
        dataCollectionCommand.exec = false;
        cmsRestartCommand.bid = 255;
        cmsRestartCommand.restart = 1;
        ledAnimation.restart();
        // restartCMSViaPin();
    }
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

int sendRequest(int bid, int sequence)
{
    int status = 0;
    LedData ledData;
    LedCommand _ledCommand;
    switch(sequence) {
    case VCELL:
        sendVcellRequest(bid);
        status = 1;
        break;
    case TEMP:
        sendTempRequest(bid);
        status = 1;
        break;
    case VPACK:
        sendVpackRequest(bid);
        status = 1;
        break;  
    case CMSSTATUS:
        sendCMSStatusRequest(bid);
        status = 1;
        break;
    case CMSREADBALANCINGSTATUS:
        sendCMSReadBalancingStatus(bid);
        status = 1;
        break;
    case LED:
        if (ledAnimation.isRunning())
        {
            ledData = ledAnimation.update();
            if(ledData.currentGroup >= 0)
            {
                convertLedDataToLedCommand(ledData, _ledCommand);
            }
            sendLedRequest(_ledCommand); //local variable ledCommand
        }
        else
        {
            sendLedRequest(ledCommand); //global variable ledCommand
        }
        status = 1;
        break;
    case CMSINFO:
        sendCMSInfoRequest(bid);
        status = 1;
        break;   
    case CMSFRAMEWRITE:
        sendCMSFrameWriteRequest(frameWrite);
        status = 1;
        break;
    case CMSCODEWRITE:
        sendCMSCodeWriteRequest(cmsCodeWrite);
        status = 1;
        break;
    case CMSBASECODEWRITE:
        sendCMSBaseCodeWriteRequest(baseCodeWrite);
        status = 1;
        break;
    case CMSMCUCODEWRITE:
        sendCMSMcuCodeWriteRequest(mcuCodeWrite);
        status = 1;
        break;
    case CMSSITELOCATIONWRITE:
        sendCMSSiteLocationWriteRequest(siteLocationWrite);
        status = 1;
        break;
    case BALANCINGWRITE:
        sendBalancingWriteRequest(cellBalancingCommand);
        status = 1;
        break;
    case SHUTDOWN:
        sendCMSShutDownRequest(cmsShutDown);
        status = 1;
        break;  
    case WAKEUP:
        sendCMSWakeupRequest(cmsWakeup);
        status = 1;
        break;
    case RESTART:
        sendCMSRestartRequest(cmsRestartCommand);
        status = 1;
        break;
    }
    return status;
}

int checkResponse(const String &input)
{
    int status = 0;
    Serial.println(input);
    if(readVcell(input) >= 0)
    {
        // Serial.println("VCELL");
        status = 1;
    }
    else if (readTemp(input) >= 0)
    {
        // Serial.println("TEMP");
        status = 1;
    }
    else if (readVpack(input) >= 0)
    {
        // Serial.println("VPACK");
        status = 1;
    }
    else if (readCMSInfo(input) >= 0)
    {
        // Serial.println("CMSINFO");
        status = 1;
    }
    else if (readFrameWriteResponse(input) >= 0)
    {
        // Serial.println("FRAME");
        status = 1;
    }
    else if (readCMSCodeWriteResponse(input) >= 0)
    {
        // Serial.println("FRAME");
        status = 1;
    }
    else if (readCMSBaseCodeWriteResponse(input) >= 0)
    {
        // Serial.println("FRAME");
        status = 1;
    }
    else if (readCMSMcuCodeWriteResponse(input) >= 0)
    {
        // Serial.println("FRAME");
        status = 1;
    }
    else if (readCMSSiteLocationWriteResponse(input) >= 0)
    {
        // Serial.println("FRAME");
        status = 1;
    }
    else if (readCMSBalancingResponse(input) >= 0)
    {
        // Serial.println("BAL");
        status = 1;
    }
    else if (readCMSBQStatusResponse(input) >= 0)
    {
        // Serial.println("BQSTAT");
        status = 1;
    }
    else if (readLed(input) >= 0)
    {
        // Serial.println("LED");
        status = 1;
    }
    else if (readAddressing(input) >= 0)
    {
        // Serial.println("ADDRESS RESPONSE");
        status = 1;
    }
    else if(readAddressingResponse(input) >= 0)
    {
        // Serial.println("ADDRESS STATUS");
        status = 1;
    }
    return status;
}

void addressing(bool isFromBottom)
{
    isAddressingCompleted = 0;
    int bidAddr = 1; // bid address start from 1
    addressList.clear();
    for (int i = 0; i < numOfShiftRegister; i++)
    {
        bool isDataComplete = false;
        String serialData;
        uint8_t x;
        if(isFromBottom)
        {
            x = (8 * (numOfShiftRegister - i)) - 1; // Q7 is connected to addressing pin of CMS
        }
        else
        {
            x = (8 * i) + 7;
        }
        
        Serial.print("number = ");
        Serial.println(x);
        sr.set(x, HIGH);
        digitalWrite(internalLed,HIGH);
        delay(200);
        sr.set(x, LOW);
        digitalWrite(internalLed,LOW);
        delay(200);
        
        while (Serial2.available()) //get for the response {"BID_STATUS" : 1}
        {
            char in = Serial2.read();
            if (in == '\n')
            {
                isDataComplete = true;
            }
            else
            {
                serialData += in;
            }
        }
        
        if (isDataComplete)
        {
            // Serial.println(serialData);
            if(checkResponse(serialData)) //if the response {"BID_STATUS" : 1}
            {
                StaticJsonDocument<128> doc;
                String output;
                doc["BID_ADDRESS"] = bidAddr;
                doc["SR"] = x;
                serializeJson(doc, output);
                Serial2.print(output);
                Serial2.print('\n');
                Serial.print("EHUB Number -> BMS === ");
                Serial.println(bidAddr);
                bidAddr++;
            }
        }
        else
        {
            Serial.println("No Response, Skip!");
            continue;
        }
        serialData = "";
        isDataComplete = false;
        delay(100);

        while (Serial2.available()) //check the addressing response {"BID" : n, "RESPONSE" : 1}
        {
            char in = Serial2.read();
            if (in == '\n')
            {
                isDataComplete = true;
            }
            else
            {
                serialData += in;
            }
        }
        
        if (isDataComplete)
        {
            // Serial.println(serialData);
            checkResponse(serialData);
        }
        Serial.println("===============xxxxxxxxx===========");
        // Serial.println(sr.get(x));
        // delay(200);
        // sr.setAllLow();
    }
    isAddressingCompleted = 1;
    digitalWrite(internalLed,HIGH);
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
    //     fillArrayRandom<int>(cellData[i].vcell, 45, 2800, 3800);
    //     fillArrayRandom<int32_t>(cellData[i].temp, 9, 10000, 100000);
    //     fillArrayRandom<int32_t>(cellData[i].pack, 3, 32000, 40000);
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
    Serial.println(ssid);
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
    leds[9] = CRGB::LawnGreen;
    FastLED.setBrightness(20);
    FastLED.show();
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

void setup()
{
    // nvs_flash_erase(); // erase the NVS partition and...
    // nvs_flash_init(); // initialize the NVS partition.
    // while(true);
    Serial.begin(115200);

    Preferences preferences;
    preferences.begin("dev_params");
    uint8_t nFlag = preferences.getChar("n_flag");
    uint8_t pFlag = preferences.getChar("p_flag");
    Serial.println("n_flag : " + String(nFlag));
    Serial.println("p_flag : " + String(nFlag));
    preferences.end();
    if ((nFlag == 0) || (pFlag == 0))
    {
        Serial.println("Initialized default and user value");
        setDefaultPreference();
        setUserPreference();
    }

    preferences.begin("dev_params");
    preferences.putChar("n_flag", 2);
    preferences.putChar("p_flag", 2);
    preferences.end();

    settingRegisters.link(&alarmParam.vcell_diff, 0);
    settingRegisters.link(&alarmParam.vcell_diff_reconnect, 1);
    settingRegisters.link(&alarmParam.vcell_max, 2);
    settingRegisters.link(&alarmParam.vcell_min, 3);
    settingRegisters.link(&alarmParam.vcell_reconnect, 4);
    alarmParam.temp_max = 100000;
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
    
    bool *boolPtr = reinterpret_cast<bool*>(&addressingCommand.exec);
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

    modbusRegisterData.inputRegister.cellData = cellData;
    modbusRegisterData.inputRegister.cellDataSize = cellDataSize;
    modbusRegisterData.inputRegister.otherInfo = &otherInfo;
    modbusRegisterData.holdingRegisters.settingRegisters = &settingRegisters;
    modbusRegisterData.mbusCoil.mbusCoilData = &mbusCoilData;

    startButton.attachClick(buttonClicked);
    startButton.attachLongPressStart(buttonLongPressed);
    // startButton.attachDoubleClick(buttonDoubleClicked);
    startButton.setDebounceTicks(50);
    EEPROM.begin(256);
    if(EEPROM.read(EEPROM_RMS_ADDRESS_CONFIGURED_FLAG) == 1)
    {
        rmsCode = EEPROM.readString(EEPROM_RMS_CODE_ADDRESS);
    }

    if(EEPROM.read(EEPROM_RACK_SN_CONFIGURED_FLAG) == 1)
    {
        rackSn = EEPROM.readString(EEPROM_RACK_SN_ADDRESS);
    }
    myTimer = timerBegin(0, 80, true);
    timerAttachInterrupt(myTimer, &onTimer, true);
    timerAlarmWrite(myTimer, 500000, true);
    pinMode(battRelay, OUTPUT);
    pinMode(buzzer, OUTPUT);
    pinMode(internalLed, OUTPUT);
    

    Serial2.setRxBufferSize(1024);
    Serial2.begin(115200);
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
    WiFi.disconnect(true);
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.mode(WIFI_MODE_NULL);
    delay(100);
    WiFi.setHostname(hostName.c_str());
    // WiFi.mode(WIFI_STA);

    String networkSsid;
    String pwd;
    IPAddress ipAddr;
    IPAddress gatewayAddr;
    IPAddress subnetAddr;
    uint8_t mode;
    uint8_t serverType;

    preferences.begin("dev_params");

    if (preferences.getChar("n_flag") == 1)
    {
        networkSsid = preferences.getString("d_ssid");
        pwd = preferences.getString("d_pass");
        ipAddr.fromString(preferences.getString("d_ip"));
        gatewayAddr.fromString(preferences.getString("d_gateway"));
        subnetAddr.fromString(preferences.getString("d_subnet"));
        serverType = preferences.getChar("d_server");
        mode = preferences.getChar("d_mode");
        // Serial.println("=====Default=====");
        // Serial.println("SSID : " + ssid);
        // Serial.println("Pass : " + password);
        // Serial.println("Ip : " + ipAddress);
        // Serial.println("Gateway : " + gateways);
        // Serial.println("Subnet : " + subnets);
        switch (mode)
        {
            case Network::MODE::AP :
                // Serial.println("Default AP");
                WiFi.mode(WIFI_AP);
            break;
            case Network::MODE::STATION :
                // Serial.println("Default Station");
                WiFi.mode(WIFI_STA);
            break;
            default:
            break;
        }

        switch (serverType)
        {
            case Network::Server::STATIC :
            // Serial.println("Default Static");

            if (mode == Network::MODE::STATION)
            {
                if (!WiFi.config(ipAddr, gatewayAddr, subnetAddr))
                {
                    Serial.println("STA Failed to configure");
                }
            }
            else
            {
                if (!WiFi.softAPConfig(ipAddr, gatewayAddr, subnetAddr))
                {
                Serial.println("AP Failed to configure");
                }
            }
            break;

            case Network::Server::DHCP :
                // Serial.println("Default Dynamic");
            break;
            
            default:
            break;
        }
    }
    else
    {
        networkSsid = preferences.getString("ssid");
        pwd = preferences.getString("pass");
        ipAddr.fromString(preferences.getString("ip"));
        gatewayAddr.fromString(preferences.getString("gateway"));
        subnetAddr.fromString(preferences.getString("subnet"));
        serverType = preferences.getChar("server");
        mode = preferences.getChar("mode");

        uint16_t temp[8];
        size_t resultLength = SettingRegisters::stringToDoubleChar(networkSsid, temp, 8);
        settingRegisters.setBulk(10, temp, resultLength); //SSID register
        resultLength = SettingRegisters::stringToDoubleChar(pwd, temp, 8);
        settingRegisters.setBulk(18, temp, resultLength);
        // Serial.println("=====User=====");
        // Serial.println("SSID : " + ssid);
        // Serial.println("Pass : " + password);
        // Serial.println("Ip : " + ipAddress);
        // Serial.println("Gateway : " + gateways);
        // Serial.println("Subnet : " + subnets);
        switch (mode)
        {
            case Network::MODE::AP :
                // Serial.println("User AP");
                WiFi.mode(WIFI_AP);
            break;
            case Network::MODE::STATION :
                // Serial.println("User Station");
                WiFi.mode(WIFI_STA);
            break;
            default:
            break;
        }

        switch (serverType)
        {
            case Network::Server::STATIC :
                if (!WiFi.config(ipAddr, gatewayAddr, subnetAddr))
                {
                    Serial.println("STA Failed to configure");
                }
            break;
            case Network::Server::DHCP :
                // Serial.println("User Dynamic");
            break;
            
            default:
            break;
        }
    }
    
    preferences.end();

    #ifdef DEBUG_STATIC    
        if (!WiFi.config(local_ip, gateway, subnet))
        {
            Serial.println("STA Failed to configure");
        }
    #endif
    
    WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
    // WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    Serial.print("Try connecting to ");
    Serial.println(networkSsid);

    int timeout = 0;
    if (mode == Network::MODE::STATION)
    {
        WiFi.begin(networkSsid, pwd);
        while (WiFi.status() != WL_CONNECTED)
        {
            if (timeout >= 10)
            {
                Serial.println("Failed to connect into " + networkSsid);
                break;
            }
            Serial.print(".");
            delay(500);
            timeout++;
        }
    }
    else
    {
        WiFi.softAP(ssid,password);
        Serial.println("AP Connected");
        Serial.print("SSID : ");
        Serial.println(networkSsid);
        Serial.print("IP address: ");
        Serial.println(WiFi.softAPIP().toString());
        Serial.print("Subnet Mask: ");
        Serial.println(subnetAddr);
        Serial.print("Gateway IP: ");
        Serial.println(gatewayAddr);
        Serial.print("Hostname: ");
        Serial.println(WiFi.softAPgetHostname());
        digitalWrite(internalLed, HIGH);
    }

    delay(100); //wait a bit to stabilize voltage and current
    if (!MDNS.begin(hostName.c_str())) {             // Start the mDNS responder for esp8266.local
        Serial.println("Error setting up MDNS responder!");
    }
    Serial.println("mDNS responder started");
    delay(100);
    // Wire.begin(I2C_SDA, I2C_SCL);
    // lcd.init();
    // lcd.backlight();    
    // lcd.clear();

    if (timeout >= 10)
    {
        Serial.println("WiFi Not Connected");
        digitalWrite(internalLed, LOW);
    }

    activeMode = mode;

    timerAlarmEnable(myTimer);
    delay(100); //wait a bit to stabilize the voltage and current consumption
    digitalWrite(battRelay, HIGH);
    // digitalWrite(relay[1], HIGH);
    // setShiftRegisterState();
    #ifdef HARDWARE_ALARM
        hardwareAlarm.enable = 1;
    #endif
    declareStruct();
    fillArray<int8_t>(isDataNormalList, 12, -1);
    ledAnimation.setLedGroupNumber(addressList.size());
    ledAnimation.setLedStringNumber(8);
    ledAnimation.run();
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
        data.rackSn = rackSn;
        String jsonOutput = jsonManager.buildJsonData(request, data, cellDataArrSize);
        request->send(200, "application/json", jsonOutput); });

    server.on("/get-data", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        String buffer;
        data.rackSn = rackSn;
        if(jsonManager.buildJsonData(request, data, buffer))
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
        size_t cmsInfoArrSize = sizeof(cmsInfo) / sizeof(cmsInfo[0]);
        // Serial.println(cmsInfoArrSize);
        String jsonOutput = jsonManager.buildJsonCMSInfo(cmsInfo, cmsInfoArrSize);
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
      switch (activeMode)
      {
      case Network::MODE::STATION :
        s.ssid = WiFi.SSID();
        s.ip = WiFi.localIP().toString();
        break;
      case Network::MODE::AP :
        s.ssid = WiFi.softAPSSID();
        s.ip = WiFi.softAPIP().toString();
        break;
      default:
        break;
      }
      request->send(200, "application/json", jsonManager.getNetworkInfo(s)); });

    server.on("/get-user-network-setting", HTTP_GET, [](AsyncWebServerRequest *request)
    {
      Serial.println("Get user network setting info");
      NetworkSetting s;
      Preferences preferences;
      preferences.begin("dev_params");
      s.ssid = preferences.getString("ssid");
      s.pass = preferences.getString("pass");
      s.ip = preferences.getString("ip");
      s.gateway = preferences.getString("gateway");
      s.subnet = preferences.getString("subnet");
      s.mode = preferences.getChar("mode");
      s.server = preferences.getChar("server");
      preferences.end();
      request->send(200, "application/json", jsonManager.getUserNetworkSetting(s)); });

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
        request->send(200, "application/json", response);});

    AsyncCallbackJsonWebHandler *setAddressHandler = new AsyncCallbackJsonWebHandler("/set-addressing", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonAddressingCommandParser(input.c_str());
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
        request->send(200, "application/json", response); });

    AsyncCallbackJsonWebHandler *setAlarmParamHandler = new AsyncCallbackJsonWebHandler("/set-alarm-parameter", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonAlarmParameterParser(input.c_str(), alarmParam);
        response.replace(":status:", String(status));
        request->send(200, "application/json", response); });

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
        request->send(200, "application/json", response); });

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
        request->send(200, "application/json", response); });
    
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
        request->send(200, "application/json", response); });
    
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
            status = writeToEeprom(EEPROM_RMS_CODE_ADDRESS, EEPROM_RMS_ADDRESS_CONFIGURED_FLAG, rmsCodeWrite.rmsCode, rmsCode);
            if(status)
            {
                rmsInfo.rmsCode = rmsCodeWrite.rmsCode;
            }
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
            status = writeToEeprom(EEPROM_RACK_SN_ADDRESS, EEPROM_RACK_SN_CONFIGURED_FLAG, rmsRackSnWrite.rackSn, rackSn);
            if(status)
            {
                rmsInfo.rackSn = rmsRackSnWrite.rackSn;
            }
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
        request->send(200, "application/json", response); });

    AsyncCallbackJsonWebHandler *setCmsCodeHandler = new AsyncCallbackJsonWebHandler("/set-cms-code", [](AsyncWebServerRequest *request, JsonVariant &json)
    {
        String response = R"(
        {
        "status" : :status:
        }
        )";
        String input = json.as<String>();
        int status = jsonManager.jsonCMSCodeParser(input.c_str(), cmsCodeWrite);
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
        request->send(200, "application/json", response); });

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
        request->send(200, "application/json", response); });

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
        request->send(200, "application/json", response); });

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
        Preferences preferences;
        preferences.begin("dev_params");
        if (setting.flag > 0)
        {
            Serial.println("Set network");
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
        preferences.end();
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
    // server.addHandler(restartCMSViaPinHandler);

    MBserver.registerWorker(1, READ_COIL, &FC01);      // FC=01 for serverID=1
    MBserver.registerWorker(1, READ_HOLD_REGISTER, &FC03);      // FC=03 for serverID=1
    MBserver.registerWorker(1, READ_INPUT_REGISTER, &FC04);     // FC=04 for serverID=1
    MBserver.registerWorker(1, WRITE_COIL, &FC05);     // FC=05 for serverID=1
    MBserver.registerWorker(1, WRITE_HOLD_REGISTER, &FC06);      // FC=06 for serverID=1
    MBserver.registerWorker(1, WRITE_MULT_REGISTERS, &FC16);    // FC=16 for serverID=1

    MBserver.start(502, 10, 20000);

    // AsyncElegantOTA.begin(&server); // Start ElegantOTA
    server.begin();
    resetUpdater();
    Serial.println("HTTP server started");
    lastReconnectMillis = millis();
    // restartCMSViaPin();
    // delay(100);
    dataCollectionCommand.exec = false;
    cmsRestartCommand.bid = 255;
    cmsRestartCommand.restart = 1;
    // for (size_t i = 0; i < 12; i++)
    // {
    //     Serial.println("Data normal list : " + String(isDataNormalList[i]));
    // }
    delay(2000); //wait for CMS to boot
    systemStatus.bits.ready = 1;
    testDataCollection.exec = 1;
}

void loop()
{
    int isRxBufferEmpty = false;    
    int isResetSerialTimer = false;
    int serialResponse = 0;
    int qty;
    startButton.tick();
    // LedData ledData = ledAnimation.update();
    // Serial.println("Current Group : " + String(ledData.currentGroup));
    // Serial.println("Current String : " + String(ledData.currentString));
    // Serial.println("Address List Size : " + String(addressList.size()));
    
    if ((WiFi.status() != WL_CONNECTED) && (millis() - lastReconnectMillis >= reconnectInterval)) {
        digitalWrite(internalLed, LOW);
        Serial.println("Reconnecting to WiFi...");
        WiFi.disconnect();
        WiFi.reconnect();
        lastReconnectMillis = millis();
    }

    if (manualOverride)
    {
        // Serial.println("Manual Override");
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
        Preferences preferences;
        preferences.begin("dev_params");
        preferences.putChar("n_flag", 1);
        preferences.putChar("p_flag", 1);
        preferences.end();
        Serial.println("Factory Reset");
        delay(500);
        ESP.restart();
    }

    // digitalWrite(relay[0], !alarmCommand.powerRelay);
    // digitalWrite(relay[1], !alarmCommand.battRelay);
    if(Serial.available())
    {
        char c = Serial.read();
        if (c == '\n' || c == '\r')
        {
            while (Serial.available())
            {
                Serial.read();
            }
            
            commandCompleted = true;
        }
        else
        {
            commandString += c;
        }
    }

    if (Serial2.available())
    {
        char c = Serial2.read();
        if (c == '\n')
        {
            responseCompleted = true;
        }
        else
        {
            responseString += c;
        }
        // lastTime = millis();
        lastReceivedSerialData = millis();
        isResetSerialTimer = true;
    }

    if (commandCompleted)
    {
        Serial.println(commandString);
        #ifndef DEBUG
            evalCommand(commandString);
        #endif
        checkResponse(commandString);
        commandCompleted = false;
        commandString = "";
    }

    if (responseCompleted)
    {
        Serial.println(responseString);
        serialResponse = checkResponse(responseString);
        // if (evalResponse(responseString))
        // {
        //     commandSequence++;
        //     sendCommand = true;
        //     lastTime = millis();
        // }
        responseCompleted = false;
        responseString = "";
    }

    if (lastDeviceAddress != deviceAddress)
    {
        dataComplete = 0;
        lastDeviceAddress = deviceAddress;
    }
    else
    {
        dataComplete += serialResponse;
    }
    
    for(int i = 0; i < addressList.size(); i++)
    {
        int isUpdate = 0;
        isUpdate = updater[addressList.at(i) - 1].isUpdate();
        // Serial.println("Device Address : " + String(addressList.at(i)));
        // Serial.println("is Update = " + String(isUpdate));
        if(isUpdate)
        {            
            int isDataNormal = updater[addressList.at(i) - 1].isDataNormal();
            isDataNormalList[addressList.at(i) - 1] = isDataNormal;
            Serial.println("Bid : " + String(addressList.at(i) - 1));
            Serial.println("Normal : "  + String(isDataNormal));
            Serial.println("Data is Complete.. Pushing to Database");
            msgCount[i]++;
            cellData[i].msgCount = msgCount[i];
            data.rackSn = rackSn;
            // systemStatus.val = systemStatus.val | cellData[i].packStatus.val;
            #ifdef AUTO_POST
                String phpName = "update.php";
                String link = serverName + phpName;
                HTTPClient http;
                http.begin(link);
                http.addHeader("Content-Type", "application/json");
                String httpPostData = jsonManager.buildSingleJsonData(cellData[addressList.at(i) - 1]);
                Serial.println("Device Address : " + String(addressList.at(i)));
                Serial.println(httpPostData);
                int httpResponseCode = http.POST(httpPostData);
                Serial.print("HTTP Response code: ");
                Serial.println(httpResponseCode);
                http.end();
            #endif
            updater[addressList.at(i) - 1].resetUpdater();    
        }
        
    }

    if(dataCollectionCommand.exec)
    // if (testDataCollection.exec)
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
                    // for (size_t i = 0; i < 12; i++)
                    // {
                    //     Serial.println("Data normal list : " + String(isDataNormalList[i]));
                    // }
                    
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
                // Serial.println("Data Normal : " + String(dataNormal));
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

            // if(isDataNormalListUpdated)
            // {
            //     Serial.println("Buzzer state : " + String(buzzerState));
            // }
        }
    }
    

    if (commandSequence > 4)
    {
        commandSequence = 0;
        deviceAddress++;
    }
    if (deviceAddress >= addressList.size())
    {
        deviceAddress = 0;
        commandSequence = 0;
    }

    if(isResetSerialTimer)
    {
        lastReceivedSerialData = millis();
        // Serial.println("updated last serial");
    }

    if (millis() - lastReceivedSerialData > 50)
    {
        // Serial.println("No Serial Data");
        isRxBufferEmpty = true;
    }
    else
    {
        lastTime = millis();
    }

    // Serial.println("Rx Buffer Empty : " + String(isRxBufferEmpty));

    if (addressingByButton)
    {
        delay(100);
        while (Serial2.available())
        {
            Serial2.read();
        }
        
        resetUpdater();
        dataCollectionCommand.exec = 0;
        Serial.println("Doing Addressing");
        // performAddressing();
        // performAddressingTest2();
        addressing(true);
        addressingCommand.exec = 0;
        addressingByButton = 0;
        sendCommand = true;
        Serial.println("Addressing Finished");
        isGotCMSInfo = false;
        lastTime = millis();
        digitalWrite(internalLed, HIGH);
        dataCollectionCommand.exec = 1;
        timerStart(myTimer);
    }

    if (addressingCommand.exec)
    {
        // perform addressing
        // restartCMSViaPin();
        // delay(1000);
        delay(100);
        while (Serial2.available())
        {
            Serial2.read();
        }
        
        resetUpdater();
        dataCollectionCommand.exec = 0;
        addressingByButton = 0;
        Serial.println("Doing Addressing");
        // performAddressing();
        // performAddressingTest2();
        addressing(true);
        addressingCommand.exec = 0;
        sendCommand = true;
        Serial.println("Addressing Finished");
        isGotCMSInfo = false;
        lastTime = millis();
    }
    else
    {       
        if (frameWrite.write)
        {
            Serial.println("Write Frame");
            if (lastFrameWrite != frameWrite.write) //check if there is command to write frame
            {
                lastFrameWrite = frameWrite.write;
                lastStateDataCollection = dataCollectionCommand.exec; // save the last state of data collection
            }
            // dataCollectionCommand.exec = false;
            isGotCMSInfo = false;
            deviceAddress = 0;
            commandSequence = 0;
            if (isRxBufferEmpty && !Serial2.available())
            {
                if(sendCommand)
                {
                    if (dataCollectionCommand.exec == false)
                    {
                        sendRequest(frameWrite.bid, CMSFRAMEWRITE);
                        isRxBufferEmpty = false;
                        sendCommand = false;
                        frameWrite.bid = 0;
                        frameWrite.write = false;
                        lastFrameWrite = false;
                        dataCollectionCommand.exec = lastStateDataCollection; // retrieve the last state of data collection
                        lastTime = millis();
                        lastReceivedSerialData = millis();
                    }
                    else
                    {
                        dataCollectionCommand.exec = false;
                    }
                }
                else
                {
                    dataCollectionCommand.exec = false;
                }
            }
            else
            {
                dataCollectionCommand.exec = false;
            }         
        }

        if (cmsCodeWrite.write)
        {
            Serial.println("Write CMS Code");
            if (lastCmsCodeWrite != cmsCodeWrite.write) //check if there is command to write frame
            {
                lastCmsCodeWrite = cmsCodeWrite.write;
                lastStateDataCollection = dataCollectionCommand.exec; // save the last state of data collection
            }
            // dataCollectionCommand.exec = false;
            isGotCMSInfo = false;
            deviceAddress = 0;
            commandSequence = 0;
            if (isRxBufferEmpty && !Serial2.available())
            {
                if(sendCommand)
                {
                    if (dataCollectionCommand.exec == false)
                    {
                        sendRequest(cmsCodeWrite.bid, CMSCODEWRITE);
                        isRxBufferEmpty = false;
                        sendCommand = false;
                        cmsCodeWrite.bid = 0;
                        cmsCodeWrite.write = false;
                        lastCmsCodeWrite = false;
                        dataCollectionCommand.exec = lastStateDataCollection; // retrieve the last state of data collection
                        lastTime = millis();
                        lastReceivedSerialData = millis();
                    }
                    else
                    {
                        dataCollectionCommand.exec = false;
                    }
                }
                else
                {
                    dataCollectionCommand.exec = false;
                }
            }
            else
            {
                dataCollectionCommand.exec = false;
            }         
        }

        if (baseCodeWrite.write)
        {
            Serial.println("Write Base Code");
            if (lastBaseCodeWrite != baseCodeWrite.write) //check if there is command to write frame
            {
                lastBaseCodeWrite = baseCodeWrite.write;
                lastStateDataCollection = dataCollectionCommand.exec; // save the last state of data collection
            }
            // dataCollectionCommand.exec = false;
            isGotCMSInfo = false;
            deviceAddress = 0;
            commandSequence = 0;
            if (isRxBufferEmpty && !Serial2.available())
            {
                if(sendCommand)
                {
                    if (dataCollectionCommand.exec == false)
                    {
                        sendRequest(baseCodeWrite.bid, CMSBASECODEWRITE);
                        isRxBufferEmpty = false;
                        sendCommand = false;
                        baseCodeWrite.bid = 0;
                        baseCodeWrite.write = false;
                        lastBaseCodeWrite = false;
                        dataCollectionCommand.exec = lastStateDataCollection; // retrieve the last state of data collection
                        lastTime = millis();
                        lastReceivedSerialData = millis();
                    }
                    else
                    {
                        dataCollectionCommand.exec = false;
                    }
                }
                else
                {
                    dataCollectionCommand.exec = false;
                }
            }
            else
            {
                dataCollectionCommand.exec = false;
            }         
        }

        if (mcuCodeWrite.write)
        {
            Serial.println("Write Mcu Code");
            if (lastMcuCodeWrite != mcuCodeWrite.write) //check if there is command to write frame
            {
                lastMcuCodeWrite = mcuCodeWrite.write;
                lastStateDataCollection = dataCollectionCommand.exec; // save the last state of data collection
            }
            // dataCollectionCommand.exec = false;
            isGotCMSInfo = false;
            deviceAddress = 0;
            commandSequence = 0;
            if (isRxBufferEmpty && !Serial2.available())
            {
                if(sendCommand)
                {
                    if (dataCollectionCommand.exec == false)
                    {
                        sendRequest(mcuCodeWrite.bid, CMSMCUCODEWRITE);
                        isRxBufferEmpty = false;
                        sendCommand = false;
                        mcuCodeWrite.bid = 0;
                        mcuCodeWrite.write = false;
                        lastMcuCodeWrite = false;
                        dataCollectionCommand.exec = lastStateDataCollection; // retrieve the last state of data collection
                        lastTime = millis();
                        lastReceivedSerialData = millis();
                    }
                    else
                    {
                        dataCollectionCommand.exec = false;
                    }
                }
                else
                {
                    dataCollectionCommand.exec = false;
                }
            }
            else
            {
                dataCollectionCommand.exec = false;
            }         
        }

        if (siteLocationWrite.write)
        {
            Serial.println("Write Mcu Code");
            if (lastSiteLocationWrite != siteLocationWrite.write) //check if there is command to write frame
            {
                lastSiteLocationWrite = siteLocationWrite.write;
                lastStateDataCollection = dataCollectionCommand.exec; // save the last state of data collection
            }
            // dataCollectionCommand.exec = false;
            isGotCMSInfo = false;
            deviceAddress = 0;
            commandSequence = 0;
            if (isRxBufferEmpty && !Serial2.available())
            {
                if(sendCommand)
                {
                    if (dataCollectionCommand.exec == false)
                    {
                        sendRequest(siteLocationWrite.bid, CMSSITELOCATIONWRITE);
                        isRxBufferEmpty = false;
                        sendCommand = false;
                        siteLocationWrite.bid = 0;
                        siteLocationWrite.write = false;
                        lastSiteLocationWrite = false;
                        dataCollectionCommand.exec = lastStateDataCollection; // retrieve the last state of data collection
                        lastTime = millis();
                        lastReceivedSerialData = millis();
                    }
                    else
                    {
                        dataCollectionCommand.exec = false;
                    }
                }
                else
                {
                    dataCollectionCommand.exec = false;
                }
            }
            else
            {
                dataCollectionCommand.exec = false;
            }         
        }

        if (cellBalancingCommand.sbal)
        {
            Serial.println("Do Balancing");
            if (lastCellBalancingSball != cellBalancingCommand.sbal) //check if there is command to write balancing
            {
                lastCellBalancingSball = cellBalancingCommand.sbal;
                lastStateDataCollection = dataCollectionCommand.exec; // save the last state of data collection
            }
            isGotCMSInfo = false;
            deviceAddress = 0;
            commandSequence = 0;
            if (isRxBufferEmpty && !Serial2.available())
            {
                if(sendCommand)
                {
                    if (dataCollectionCommand.exec == false)
                    {
                        sendRequest(cellBalancingCommand.bid, BALANCINGWRITE);
                        isRxBufferEmpty = false;
                        sendCommand = false;
                        cellBalancingCommand.bid = 0;
                        cellBalancingCommand.sbal = false;
                        lastCellBalancingSball = false;
                        dataCollectionCommand.exec = lastStateDataCollection;
                        lastTime = millis();
                        lastReceivedSerialData = millis();
                    }
                    else
                    {
                        dataCollectionCommand.exec = false;
                    }
                }
                else
                {
                    dataCollectionCommand.exec = false;
                }
                    
            }
            else
            {
                dataCollectionCommand.exec = false;
            }
        }

        if (ledCommand.ledset)
        {
            Serial.println("Set Led");
            if (lastLedset != ledCommand.ledset) //check if there is command to write balancing
            {
                lastLedset = ledCommand.ledset;
                lastStateDataCollection = dataCollectionCommand.exec; // save the last state of data collection
            }
            isGotCMSInfo = false;
            deviceAddress = 0;
            commandSequence = 0;
            if (isRxBufferEmpty && !Serial2.available())
            {
                if(sendCommand)
                {
                    if (dataCollectionCommand.exec == false)
                    {
                        sendRequest(ledCommand.bid, LED);
                        isRxBufferEmpty = false;
                        sendCommand = false;
                        ledCommand.bid = 0;
                        ledCommand.ledset = false;
                        for (size_t i = 0; i < 8; i++)
                        {
                            ledCommand.red[i] = 0;
                            ledCommand.green[i] = 0;
                            ledCommand.blue[i] = 0;
                        }
                        lastLedset = false;
                        dataCollectionCommand.exec = lastStateDataCollection;
                        lastTime = millis();
                        lastReceivedSerialData = millis();
                    }
                    else
                    {
                        dataCollectionCommand.exec = false;
                    }
                }
                else
                {
                    dataCollectionCommand.exec = false;
                }
                    
            }
            else
            {
                dataCollectionCommand.exec = false;
            }
        }

        if (cmsShutDown.shutdown)
        {
            Serial.println("Shutdown CMS");
            if(lastCMSShutdown != cmsShutDown.shutdown)
            {
                lastCMSShutdown = cmsShutDown.shutdown;
                lastStateDataCollection = dataCollectionCommand.exec;
            }
            // dataCollectionCommand.exec = false;
            isGotCMSInfo = false;
            deviceAddress = 0;
            commandSequence = 0;

            if (isRxBufferEmpty && !Serial2.available())
            {
                if(sendCommand)
                {
                    if(dataCollectionCommand.exec == false)
                    {
                        sendRequest(cmsShutDown.bid, SHUTDOWN);
                        isRxBufferEmpty = false;
                        sendCommand = false;
                        cmsShutDown.bid = 0;
                        cmsShutDown.shutdown = false;
                        lastCMSShutdown = false;
                        dataCollectionCommand.exec = lastStateDataCollection;
                        lastTime = millis();
                        lastReceivedSerialData = millis();
                    }
                    else
                    {
                        dataCollectionCommand.exec = false;
                    }
                }
                else
                {
                    dataCollectionCommand.exec = false;
                }
            }
            else
            {
                dataCollectionCommand.exec = false;
            }
        }

        if (cmsWakeup.wakeup)
        {
            Serial.println("Wakeup CMS");
            
            if(lastCMSWakeup != cmsWakeup.wakeup)
            {
                lastCMSWakeup = cmsWakeup.wakeup;
                lastStateDataCollection = dataCollectionCommand.exec;
            }

            isGotCMSInfo = false;
            deviceAddress = 0;
            commandSequence = 0;
            
            if (isRxBufferEmpty && !Serial2.available())
            {
                if(sendCommand)
                {
                    if(dataCollectionCommand.exec == false)
                    {
                        sendRequest(cmsWakeup.bid, WAKEUP);
                        isRxBufferEmpty = false;
                        sendCommand = false;
                        cmsWakeup.bid = 0;
                        cmsWakeup.wakeup = false;
                        lastCMSWakeup = false;
                        dataCollectionCommand.exec = lastStateDataCollection;
                        lastTime = millis();
                        lastReceivedSerialData = millis();
                    }
                    else
                    {
                        dataCollectionCommand.exec = false;
                    }
                }
                else
                {
                    dataCollectionCommand.exec = false;
                }    
            }
            else
            {
                dataCollectionCommand.exec = false;
            }
            
        }

        if (cmsRestartCommand.restart)
        {
            Serial.println("Restart CMS By Http Request");
            isGotCMSInfo = false;
            deviceAddress = 0;
            commandSequence = 0;
            if (isRxBufferEmpty && !Serial2.available())
            {
                if(sendCommand)
                {
                    if (dataCollectionCommand.exec == false)
                    {
                        // Serial.println("Restart CMS Block");
                        resetUpdater();
                        sendRequest(cmsRestartCommand.bid, RESTART);
                        isRxBufferEmpty = false;
                        sendCommand = false;
                        cmsRestartCommand.bid = 0;
                        cmsRestartCommand.restart = false;
                        reInitCellData();                   
                        lastTime = millis();
                        lastReceivedSerialData = millis();
                    }
                    else
                    {
                        // Serial.println("Else Restart CMS Block");
                        dataCollectionCommand.exec = false;
                    }
                }
                else
                {
                    dataCollectionCommand.exec = false;
                }
                    
            }
            else
            {
                dataCollectionCommand.exec = false;
            }
        }

        if (isCmsRestartPin)
        {
            Serial.println("Restart CMS");
            isGotCMSInfo = false;
            deviceAddress = 0;
            commandSequence = 0;
            if (isRxBufferEmpty && !Serial2.available())
            {
                if(sendCommand)
                {
                    if (dataCollectionCommand.exec == false)
                    {
                        resetUpdater();
                        cmsRestartCommand.bid = 255;
                        cmsRestartCommand.restart = 1;
                        sendRequest(cmsRestartCommand.bid, RESTART);
                        cmsRestartCommand.bid = 0;
                        cmsRestartCommand.restart = 0;
                        isRxBufferEmpty = false;
                        // restartCMSViaPin();
                        sendCommand = false;
                        isCmsRestartPin = false;   
                        reInitCellData();              
                        lastTime = millis();
                        lastReceivedSerialData = millis();
                    }
                    else
                    {
                        dataCollectionCommand.exec = false;
                    }
                }
                else
                {
                    dataCollectionCommand.exec = false;
                }
                    
            }
            else
            {
                dataCollectionCommand.exec = false;
            }
        }

        if (rmsRestartCommand.restart)
        {
            Serial.println("Restart RMS");
            isGotCMSInfo = false;
            deviceAddress = 0;
            commandSequence = 0;
            if (isRxBufferEmpty && !Serial2.available())
            {
                if(sendCommand)
                {
                    if (dataCollectionCommand.exec == false)
                    {
                        resetUpdater();
                        for (size_t i = 0; i < NUM_LEDS; i++)
                        {
                            leds[i] = CRGB::Black;
                        }
                        FastLED.show();
                        delay(2000);
                        rmsRestartCommand.restart = 0;
                        ESP.restart();
                    }
                    else
                    {
                        dataCollectionCommand.exec = false;
                    }
                }
                else
                {
                    dataCollectionCommand.exec = false;
                }
                    
            }
            else
            {
                dataCollectionCommand.exec = false;
            }
        }
        
        if(dataCollectionCommand.exec)
        {
            
            if (addressList.size() > 0) //check if addressing success
            {
                // Serial.println("addressList > 0");
                systemStatus.bits.condition = 1;
                if (sendCommand)
                {
                    // Serial.println("send command");
                    if (isGotCMSInfo)
                    {
                        // Serial.println("got cms info");
                        if (isRxBufferEmpty && !Serial2.available())
                        {
                            // Serial.println("send request data");
                            // Serial.println("Cycle : " + String(cycle));
                            if(!cycle)
                            {
                                // Serial.println("command sequence");
                                sendRequest(addressList.at(deviceAddress), commandSequence);
                                isFromSequence = true;
                            }
                            else
                            {
                                sendRequest(addressList.at(deviceAddress), LED);
                                isFromSequence = false;
                            }
                            if(isGotCMSInfo)
                            {
                                if(ledAnimation.isRunning())
                                {
                                    cycle = !cycle;
                                }
                                else
                                {
                                    cycle = false;
                                }
                            
                            }                            
                            isRxBufferEmpty = false;
                            lastTime = millis();
                            lastReceivedSerialData = millis();
                            sendCommand = false;
                        }                
                    }
                    else
                    {
                        // Serial.println("dont have cms info");
                        if(isRxBufferEmpty && !Serial2.available())
                        {
                            // Serial.println("send cms info request");
                            sendRequest(addressList.at(deviceAddress), CMSINFO);
                            isRxBufferEmpty = false;
                            sendCommand = false;
                            if (deviceAddress >= (addressList.size() - 1)) //check if deviceAddress is in the last index
                            {
                                if (cmsInfoRetry >= 2)
                                {
                                    isGotCMSInfo = true;
                                    cmsInfoRetry = 0;
                                    deviceAddress = 0;
                                    // commandSequence = 0;
                                    lastTime = millis();
                                    lastReceivedSerialData = millis();
                                    // sendCommand = true;
                                }
                                else
                                {
                                    cmsInfoRetry++;
                                }
                            }
                            lastTime = millis();
                            lastReceivedSerialData = millis();
                        }
                        else
                        {
                            lastTime = millis();
                        }
                    }
                    
                }

                // change command every 100 ms
                if(!sendCommand)
                {
                    if ((millis() - lastTime > 100) && isRxBufferEmpty == true)
                    {
                        if (lastIsGotCmsInfo != isGotCMSInfo) // check if the flow is after request info, to prevent increment the commandSequence
                        {
                            lastIsGotCmsInfo = isGotCMSInfo;
                        }
                        else 
                        {
                            if (isGotCMSInfo) //if got cms info, sequencing command
                            {
                                // Serial.println("Send Command timeout!");
                                if(isFromSequence)
                                {
                                    commandSequence++;
                                }                           
                            } 
                            else
                            {
                                commandSequence = 0;
                                deviceAddress++; //used to retrieve cms info
                            }
                        }               
                        sendCommand = true;
                        lastTime = millis();
                        lastReceivedSerialData = millis();
                    }
                }
            }
            
        }
        else
        {
            systemStatus.bits.condition = 0;
            resetUpdater();
            // if (!testDataCollection.exec)
            // {
            //     resetUpdater();
            // }
            sendCommand = true;
            isGotCMSInfo = false;
            deviceAddress = 0;
            commandSequence = 0;
            lastIsGotCmsInfo = false;
            lastTime = millis();
            cycle = false;
            isFromSequence = false;
        }
        
    }

    // delay(100);
} // void loop end
