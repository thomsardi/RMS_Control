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

#define RXD2 16
#define TXD2 17

#define I2C_SDA 33
#define I2C_SCL 32

#define LED_PIN 27

#define NUM_LEDS 10

// #define AUTO_POST 1 //comment to disable server auto post

#define GREEN_BOARD 1 //uncomment to use green board laminate room

#ifdef GREEN_BOARD
    #define SERIAL_DATA 12
    #define SHCP 14
    #define STCP 13
#else
    #define SERIAL_DATA 14
    #define SHCP 13
    #define STCP 12
#endif

int SET;
int cell[45];
int32_t temp[9];
int32_t vpack[4]; // index 0 is total vpack
int BACK;
int state = 0;
int emergency;
int reset = 0;
int alert = 0;
int relay[] = {22, 23};
int buzzer = 19;
// create a global shift register object
// parameters: <number of shift registers> (data pin, clock pin, latch pin)

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

DynamicJsonDocument docBattery(1024);
CRGB leds[NUM_LEDS];
const char *ssid = "RnD_Sundaya";
// const char *ssid = "abcde";
const char *password = "sundaya22";
const char *host = "192.168.2.92";
// const char *host = "desktop-gu3m4fp";

// Set your Static IP address
IPAddress local_ip(192, 168, 2, 200);
// Set your Gateway IP address
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 2, 1);        // optional
IPAddress secondaryDNS(119, 18, 156, 10);       // optional
String hostName = "rms-battery1-rnd-room";

AsyncWebServer server(80);
JsonManager jsonManager;
RMSManager rmsManager;
LedAnimation ledAnimation;
Updater updater[8];

CellData cellData[8];
CellData cellDataToSend;
RMSInfo rmsInfo;
CMSInfo cmsInfo[8];
AlarmParam alarmParam;
CellAlarm cellAlarm[45];
CellBalancingCommand cellBalancingCommand;
AddressingCommand addressingCommand;
AlarmCommand alarmCommand;
SleepCommand sleepCommand;
DataCollectionCommand dataCollectionCommand;
CellBalancingStatus cellBalancingStatus[8];
CommandStatus commandStatus;
FrameWrite frameWrite;
CMSShutDown cmsShutDown;
CMSWakeup cmsWakeup;
LedCommand ledCommand;
CMSRestartCommand cmsRestartCommand;
RMSRestartCommand rmsRestartCommand;

enum CommandType {
    VCELL = 0,
    TEMP = 1,
    VPACK = 2,
    CMSSTATUS = 3,
    CMSREADBALANCINGSTATUS = 4,
    LED = 5,
    CMSINFO = 6,
    CMSFRAMEWRITE = 7,
    BALANCINGWRITE = 8,
    SHUTDOWN = 9,
    WAKEUP = 10,
    RESTART = 11
};

int dataComplete = 0;

int addressListStorage[12];
Vector<int> addressList(addressListStorage);

bool balancingCommand = false;
bool commandCompleted = false;
bool responseCompleted = false;
bool isFirstRun = true;
bool sendCommand = true;
bool isGotCMSInfo = false;
bool flasher = true;
bool lastStateDataCollection = false;
bool lastFrameWrite = false;
bool lastCellBalancingSball = false;
bool lastCMSShutdown = false;
bool lastCMSWakeup = false;
bool lastIsGotCmsInfo = false;
bool lastLedset = false;
bool isCmsRestartPin = false;
int isAddressingCompleted = 0;
int commandSequence = 0;
int deviceAddress = 1;
int lastDeviceAddress = 16;
int cmsInfoRetry = 0;
unsigned long lastTime = 0;
unsigned long lastBuzzer = 0;
unsigned long lastReceivedSerialData = 0;
unsigned long lastProcessResponse = 0;
String commandString;
String responseString;
String globFrameName;
String circularCommand[3] = {"readcell", "readtemp", "readvpack"};
String serverName = "http://192.168.2.92/mydatabase/";
// String serverName = "http://desktop-gu3m4fp.local/mydatabase/";

void reInitCellData()
{
    for (size_t i = 0; i < 8; i++)
    {
        cellData[i].frameName = "unspecified";
        cellData[i].bid = i + 100;
        for (size_t j = 0; j < 45; j++)
        {
            cellData[i].vcell[j] = -1;
        }
        for (size_t j = 0; j < 9; j++)
        {
            cellData[i].temp[j] = -1;
        }
        for (size_t j = 0; j < 3; j++)
        {
            cellData[i].pack[j] = -1;
        }
        cellData[i].status = 0;
    }
}

void declareStruct()
{
    for (size_t i = 0; i < 8; i++)
    {
        cellData[i].frameName = "unspecified";
        cellData[i].bid = i + 100;
        for (size_t j = 0; j < 45; j++)
        {
            cellData[i].vcell[j] = -1;
        }
        for (size_t j = 0; j < 9; j++)
        {
            cellData[i].temp[j] = -1;
        }
        for (size_t j = 0; j < 3; j++)
        {
            cellData[i].pack[j] = -1;
        }
        cellData[i].status = 0;
        cellData[i].door = 0;
    }
    rmsInfo.p_code = "1.2.10";
    rmsInfo.ver = "1.0";
    rmsInfo.ip = WiFi.localIP().toString();
    rmsInfo.mac = WiFi.macAddress();
    rmsInfo.deviceTypeName = "RMS";

    for (size_t i = 0; i < 8; i++)
    {
        cmsInfo[i].bid = i;
        cmsInfo[i].p_code = "1.1." + String(i);
        cmsInfo[i].ver = "1." + String(i);
        if (!(i % 2))
        {
            cmsInfo[i].chip = "dvc1024";
        }
        else
        {
            cmsInfo[i].chip = "bq76940";
        }
    }

    for (size_t i = 0; i < 8; i++)
    {
        addressList.push_back(i+1);
        cellBalancingStatus[i].bid = i;
        for (size_t j = 0; j < 45; j++)
        {
            cellBalancingStatus[i].cball[j] = -1;
        }
    }

    alarmParam.temp_max = 70000;
    alarmParam.temp_min = 20000;
    alarmParam.vcell_max = 3700;
    alarmParam.vcell_min = 2700;

    for (size_t i = 0; i < 45; i++)
    {
        cellAlarm[i].cell_number = i + 1;
        cellAlarm[i].alm_status = 0;
        cellAlarm[i].alm_code = 0;
    }
    commandStatus.addrCommand = 0;
    commandStatus.alarmCommand = 0;
    commandStatus.dataCollectionCommand = 0;
    commandStatus.sleepCommand = 0;
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

int readVcell(const String &input)
{
    int bid = -1;
    int status = -1;
    int led;
    int errTimeout;
    int startIndex; // bid start from 1, array index start from 0
    bool isAllDataCaptured = false;
    bool isAllDataNormal = true;
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
        
        if (docBattery.containsKey("BID"))
        {
            bid = docBattery["BID"];
            startIndex = bid - 1;
            cellData[startIndex].bid = bid;
        }
        if (docBattery.containsKey("VCELL"))
        {
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
        for (int c : cell)
        {
            if (c <= 200) //ignore the unconnected cell
            {
                isAllDataNormal = true;
            }
            else if (c >= alarmParam.vcell_min && c <= alarmParam.vcell_max) 
            {
                isAllDataNormal = true;
            }
            else
            {
                // Serial.println("Vcell min = " + String(alarmParam.vcell_min));
                // Serial.println("Vcell max = " + String(alarmParam.vcell_max));
                isAllDataNormal = false;
                // Serial.println("Abnormal Cell Voltage = " + String(c));
                break;
            }
        }

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
        updater[startIndex].updateVcell();
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
        if (docBattery.containsKey("BID"))
        {
            bid = docBattery["BID"];
            startIndex = bid - 1;
            cellData[startIndex].bid = bid;
        }
        if (docBattery.containsKey("TEMP"))
        {
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
        for (int32_t temperature : temp)
        {
            if (temperature > alarmParam.temp_max || temperature < alarmParam.temp_min)
            {
                isAllDataNormal = false;
                break;
            }
            else
            {
                isAllDataNormal = true;
            }
        }

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
        updater[startIndex].updateTemp();
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
        if (docBattery.containsKey("BID"))
        {
            bid = docBattery["BID"];
            startIndex = bid - 1;
        }
        if (docBattery.containsKey("VPACK"))
        {
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
        updater[startIndex].updateVpack();
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
            status = 1;
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
            // status = docBattery["status"];
            status = 1;
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

        if (docBattery.containsKey("p_code"))
        {
            // Serial.println("Writing Info to Local Storage");
            cmsInfo[startIndex].frameName = docBattery["frame_name"].as<String>();
            cellData[startIndex].frameName = cmsInfo[startIndex].frameName;
            cmsInfo[startIndex].bid = bid;
            cmsInfo[startIndex].p_code = docBattery["p_code"].as<String>();
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

int readAddressing(const String &input)
{
    int status = -1;
    int bid = 0;
    int respon = 0;
    StaticJsonDocument<128> doc;

    DeserializationError error = deserializeJson(doc, input);

    if (error) {
        Serial.println("Read Addressing");
        Serial.println(input);
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return status;
    }

    JsonObject object = doc.as<JsonObject>();

    if(!doc.isNull())
    {
        if(doc.containsKey("BID") && doc.containsKey("RESPONSE"))
        {
            bid = doc["BID"];
            int respon = doc["RESPONSE"];
            if (respon > 0)
            {
                addressList.push_back(bid);
                ledAnimation.setLedGroupNumber(addressList.size());
                status = 1;
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

    if (!(doc.containsKey("RBAL1.1") && doc.containsKey("RBAL2.1") && doc.containsKey("RBAL3.1")))
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

    if(!doc.containsKey("BID"))
    {
        // Serial.println("Does not contain BID");
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

    // Serial.println("GET WAKE STATUS");
    bid = doc["BID"];
    startIndex = bid - 1;
    status = doc["WAKE_STATUS"];
    door = doc["DOOR_STATUS"];
    // Serial.println("WAKE STATUS = " + String(status));
    cellData[startIndex].status = status;
    cellData[startIndex].door = door;
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

void performAddressingTest2()
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
        // String serialIn = "";
        // StaticJsonDocument<128> doc;
        // while(isRetry)
        // {
        //     // Serial.println(serialIn);
        //     if (timeout > 50)
        //     {
        //         break;
        //     }
        //     bool isJsonCompleted = false;
        //     while(Serial2.available())
        //     {
        //         char in = Serial2.read();
        //         if (in != '\n')
        //         {
        //             serialIn += in;
        //         }
        //         else
        //         {
        //             deserializeJson(doc, serialIn);
        //             isJsonCompleted = true;
        //         }
        //     }
        //     if(isJsonCompleted)
        //     {
        //         if(doc.containsKey("BID_STATUS"))
        //         {
        //             isRetry = false;
        //             Serial.println("BID : " + String(bid));
        //             DynamicJsonDocument docBattery(1024);
        //             String output;
        //             docBattery["BID"] = bid;
        //             docBattery["SR"] = x;
        //             serializeJson(docBattery, output);
        //             Serial2.print(output);
        //             Serial2.print('\n');
        //             isJsonCompleted = false;
        //             break;
        //         }
        //         else
        //         {
        //             int result = readAddressing(serialIn);
        //             Serial.println("Status Addressing = " + String(result));
        //         }
        //     }
        //     else
        //     {
        //         timeout++;
        //     }
        //     delay(10);
        // }
        // serializeJson(docBattery, Serial2);
        Serial.println("===============xxxxxxxxx===========");
        // delay(200);
        // sr.setAllLow();
    }
    isAddressingCompleted = 1;
}

void performAddressingTest()
{
    isAddressingCompleted = 0;
    
    addressList.clear();
    DynamicJsonDocument docBattery(1024);
    for (int i = 0; i < numOfShiftRegister; i++)
    {
        bool isRetry = 1;
        int timeout = 0;
        // int x = (8 * i) + 7; // 8 is number of shift register output
        int x = (8 * 0) + 7;
        int bid = i + 1; // id start from 1
        sr.set(x, HIGH);
        delay(1000);
        Serial.print("number = ");
        Serial.println(x);
        Serial.print("EHUB Number -> BMS === ");
        Serial.println(bid);
        sr.set(x, LOW);
        delay(200);
    }
    isAddressingCompleted = 1;
}

void performAddressing()
{
    isAddressingCompleted = 0;
    
    addressList.clear();
    DynamicJsonDocument docBattery(1024);
    for (int i = 0; i < numOfShiftRegister; i++)
    {
        int x = (8 * i) + 7; // 8 is number of shift register output
        int bid = i + 1; // id start from 1
        sr.set(x, HIGH);
        delay(2000);
        Serial.print("number = ");
        Serial.println(x);
        Serial.print("EHUB Number -> BMS === ");
        Serial.println(bid);
        String output;
        docBattery["BID"] = bid;
        docBattery["SR"] = x;
        serializeJson(docBattery, output);
        Serial2.print(output);
        Serial2.print('\n');
        Serial.println("===============xxxxxxxxx===========");
        delay(100);
        sr.set(x, LOW);
        delay(100);
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
        ledData = ledAnimation.update();
        if(ledData.currentGroup >= 0)
        {
            convertLedDataToLedCommand(ledData, _ledCommand);
        }
        sendLedRequest(_ledCommand); //local variable ledCommand
        // sendLedRequest(ledCommand); //global variable ledCommand
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
    if(readVcell(input) >= 0)
    {
        status = 1;
    }
    else if (readTemp(input) >= 0)
    {
        status = 1;
    }
    else if (readVpack(input) >= 0)
    {
        status = 1;
    }
    else if (readCMSInfo(input) >= 0)
    {
        status = 1;
    }
    else if (readFrameWriteResponse(input) >= 0)
    {
        status = 1;
    }
    else if (readCMSBalancingResponse(input) >= 0)
    {
        status = 1;
    }
    else if (readCMSBQStatusResponse(input) >= 0)
    {
        status = 1;
    }
    else if (readAddressing(input) >= 0)
    {
        status = 1;
    }
    return status;
}

void resetUpdater()
{
    for (size_t i = 0; i < 8; i++)
    {
        updater[i].resetUpdater();
    }
    
}

void setup()
{
    pinMode(relay[0], OUTPUT);
    pinMode(relay[1], OUTPUT);
    pinMode(buzzer, OUTPUT);
    Serial.begin(115200);
    Serial2.setRxBufferSize(1024);
    Serial2.begin(115200);
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
    WiFi.disconnect(true);
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.mode(WIFI_MODE_NULL);
    delay(100);
    WiFi.setHostname(hostName.c_str());
    WiFi.mode(WIFI_STA);
    // if (!WiFi.config(local_ip, gateway, subnet, primaryDNS, secondaryDNS))
    // {
    //     Serial.println("STA Failed to configure");
    // }
    WiFi.begin(ssid, password);
    // sr.setAllLow();
    // digitalWrite(buzzer, HIGH);
    // delay(500);
    // digitalWrite(buzzer, LOW);

    int timeout = 0;
    Serial.println("Connecting..");
    while(timeout < 10)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            // Serial2.print(".");
            Serial.print(".");
            leds[9] = CRGB::Gold;
            FastLED.setBrightness(1);
            FastLED.show();
            delay(100);
            leds[9] = CRGB::GreenYellow;
            FastLED.setBrightness(20);
            FastLED.show();
            delay(100);
            timeout++;
        }
        else
        {
            break;
        }
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
    if (timeout < 10)
    {
        leds[9] = CRGB::LawnGreen;
        FastLED.setBrightness(20);
        FastLED.show();
        Serial.println("");
        Serial.print("Connected to ");
        Serial.println(ssid);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
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
        // lcd.setCursor(0, 0);
        // lcd.print("IP :");
        // lcd.setCursor(0,1);
        // lcd.print(WiFi.localIP());
    }
    else
    {
        leds[9] = CRGB::Red;
        FastLED.setBrightness(20);
        FastLED.show();
        Serial.println("WiFi Not Connected");
        // lcd.setCursor(0,0);
        // lcd.print("Not Connected");
    }
    delay(100); //wait a bit to stabilize the voltage and current consumption
    digitalWrite(relay[0], HIGH);
    digitalWrite(relay[1], HIGH);
    setShiftRegisterState();
    declareStruct();
    ledAnimation.setLedGroupNumber(addressList.size());
    ledAnimation.setLedStringNumber(8);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
        request->send(200, "text/plain", "Hi! I am ESP32."); });

    server.on("/get-cms-data", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        size_t cellDataArrSize = sizeof(cellData) / sizeof(cellData[0]);
        String jsonOutput = jsonManager.buildJsonData(cellData, cellDataArrSize);
        request->send(200, "application/json", jsonOutput); });

    server.on("/get-device-general-info", HTTP_GET, [](AsyncWebServerRequest *request)
    {
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

    server.on("/post-test", HTTP_POST, [](AsyncWebServerRequest *request)
    {
        Serial.println("post-test");
        request->send(200, "text/plain", "Test Post"); });

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

    server.addHandler(setBalancingHandler);
    server.addHandler(setAddressHandler);
    server.addHandler(setAlarmHandler);
    server.addHandler(setDataCollectionHandler);
    server.addHandler(setAlarmParamHandler);
    server.addHandler(setSleepHandler);
    server.addHandler(setWakeupHandler);
    server.addHandler(setFrameHandler);
    server.addHandler(setLedHandler);
    server.addHandler(restartCMSHandler);
    server.addHandler(restartRMSHandler);
    server.addHandler(restartCMSViaPinHandler);

    // AsyncElegantOTA.begin(&server); // Start ElegantOTA
    server.begin();
    resetUpdater();
    Serial.println("HTTP server started");
    // restartCMSViaPin();
    // delay(100);
}

void loop()
{
    int isRxBufferEmpty = false;
    int isResetSerialTimer = false;
    int serialResponse = 0;
    int qty;
    // LedData ledData = ledAnimation.update();
    // Serial.println("Current Group : " + String(ledData.currentGroup));
    // Serial.println("Current String : " + String(ledData.currentString));
    if (alarmCommand.buzzer)
    {
        if (millis() - lastBuzzer < 1000)
        {
            digitalWrite(buzzer, flasher);
        }
        else
        {
            lastBuzzer = millis();
            flasher = !flasher;
        }
        // dataCollectionCommand.exec = false;
    }
    else
    {
        lastBuzzer = millis();
        digitalWrite(buzzer, LOW);
        flasher = false;
    }
        
    digitalWrite(relay[0], !alarmCommand.powerRelay);
    digitalWrite(relay[1], !alarmCommand.battRelay);
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
        evalCommand(commandString);
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
            Serial.println("Data is Complete.. Pushing to Database");
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
            
        }
    }

    if (commandSequence > 5)
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
    }

    if (millis() - lastReceivedSerialData > 100)
    {
        // Serial.println("No Serial 2 Data");
        isRxBufferEmpty = true;
    }
    else
    {
        lastTime = millis();
    }

    if (addressingCommand.exec)
    {
        // perform addressing
        // restartCMSViaPin();
        // delay(1000);
        resetUpdater();
        dataCollectionCommand.exec = 0;
        Serial.println("Doing Addressing");
        // performAddressing();
        performAddressingTest2();
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
                if (sendCommand)
                {
                    if (isGotCMSInfo)
                    {
                        if (isRxBufferEmpty && !Serial2.available())
                        {
                            sendRequest(addressList.at(deviceAddress), commandSequence);
                            isRxBufferEmpty = false;
                            lastTime = millis();
                            lastReceivedSerialData = millis();
                            sendCommand = false;
                        }                
                    }
                    else
                    {
                        if(isRxBufferEmpty && !Serial2.available())
                        {
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
                    if ((millis() - lastTime > 200) && isRxBufferEmpty == true)
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
                                commandSequence++;
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
            resetUpdater();
            sendCommand = true;
            isGotCMSInfo = false;
            deviceAddress = 0;
            commandSequence = 0;
            lastIsGotCmsInfo = false;
            lastTime = millis();
        }
        
    }

    // delay(100);
} // void loop end
