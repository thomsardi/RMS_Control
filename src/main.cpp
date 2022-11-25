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
// #include <AsyncElegantOTA.h>
#include <HTTPClient.h>
#include <JsonManager.h>
#include <AsyncJson.h>
#include <RMSManager.h>

#define RXD2 16
#define TXD2 17
#define LED_PIN 27
#define NUM_LEDS 10
#define merah 202, 1, 9

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

ShiftRegister74HC595<numOfShiftRegister> sr(12, 14, 13);
DynamicJsonDocument docBattery(1024);
CRGB leds[NUM_LEDS];
const char *ssid = "RnD_Sundaya";
const char *password = "sundaya22";
const char *host = "192.168.2.174";

AsyncWebServer server(80);
JsonManager jsonManager;
RMSManager rmsManager;

CellData cellData[8];
RMSInfo rmsInfo;
CMSInfo cmsInfo[8];
AlarmParam alarmParam;
CellAlarm cellAlarm[45];
CellBalancingCommand cellBalancingCommand[8];
AddressingCommand addressingCommand;
AlarmCommand alarmCommand;
SleepCommand sleepCommand;
DataCollectionCommand dataCollectionCommand;
CellBalancingStatus cellBalancingStatus[8];
CommandStatus commandStatus;

bool balancingCommand = false;

void declareStruct()
{
    for (size_t i = 0; i < 8; i++)
    {
        cellData[i].bid = i + 100;
        for (size_t j = 0; j < 45; j++)
        {
            cellData[i].vcell[j] = j * 100;
        }
        for (size_t j = 0; j < 6; j++)
        {
            cellData[i].temp[j] = j * 10;
        }
        for (size_t j = 0; j < 3; j++)
        {
            cellData[i].pack[j] = j * 10000;
        }
        cellData[i].status = 0;
    }
    rmsInfo.p_code = "1.2.10";
    rmsInfo.ver = "1.0";
    rmsInfo.ip = WiFi.localIP().toString();
    rmsInfo.mac = WiFi.macAddress();

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
        cellBalancingStatus[i].bid = i;
        for (size_t j = 0; j < 45; j++)
        {
            cellBalancingStatus[i].cball[j] = j * 10;
        }
    }

    alarmParam.temp_max = 70000;
    alarmParam.temp_min = 20000;
    alarmParam.vcell_max = 3500;
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

void vcell(int a)
{

    int check = 0;
    int checknilairusak = 0;
    int checknilaisehat = 0;
    int led = a - 1;
    int startIndex = a - 1; // bid start from 1, array index start from 0
    bool isAllDataCaptured = false;
    bool isAllDataNormal = true;
    cellData[startIndex].bid = a;

    Serial.println("Capturing Vcell Data");
    leds[led] = CRGB(227, 202, 9);
    FastLED.setBrightness(20);
    FastLED.show();

    String output = rmsManager.createJsonVcellDataRequest(a);

    // serializeJson(docBattery, Serial2);
    Serial2.print(output);
    DynamicJsonDocument docBattery(1024);
    deserializeJson(docBattery, Serial2);
    JsonObject object = docBattery.as<JsonObject>();
    if (!object.isNull())
    {
        Serial.print("masuk");
        if (docBattery.containsKey("VCELL"))
        {
            Serial.println("Contains key VCELL");
            int arrSize = sizeof(docBattery["VCELL"]) / sizeof(docBattery["VCELL"][0]);
            Serial.println("Array Size = " + String(arrSize));
            if (arrSize >= 45)
            {
                for (int i = 0; i < 45; i++)
                {
                    cell[i] = docBattery["VCELL"][i];
                    cellData[startIndex].vcell[i] = cell[i];
                }
                isAllDataCaptured = true;
                Serial.println("All Vcell Data Captured");
            }
        }
    }
    int len = sizeof(cell) / sizeof(cell[0]);
    String c = arrToStr<int>(cell, len);
    Serial.println(c);

    if (isAllDataCaptured)
    {
        /*
        String Link;
        HTTPClient http;

        Link = "http://192.168.2.174/rakbatterybiru/src/logic/bms_vcell_update.php?cell=" + String(a) + "," + String(c);
        // http.begin(client, Link);
        // http.GET();

        //  String respon = http.getString();
        //  Serial.println(respon);

        // http.end();
        Serial.println("berhasil");
        */

        for (int c : cell)
        {
            if ((c <= 200 && c >= alarmParam.vcell_min) && c <= alarmParam.vcell_max)
            {

                isAllDataNormal = true;
            }
            else
            {
                isAllDataNormal = false;
                break;
            }
        }

        if (!isAllDataNormal)
        {
            Serial.println("Vcell Data Abnormal");
            LedColor ledColor;
            ledColor.r = 200;
            ledColor.g = 0;
            ledColor.b = 0;
            String output = rmsManager.createJsonLedRequest(a, a, ledColor);
            Serial2.print(output);
            // serializeJson(docBattery, Serial2);
            delay(100);
        }
        else 
        {
            Serial.println("Vcell Data Normal");
            LedColor ledColor;
            ledColor.r = 0;
            ledColor.g = 200;
            ledColor.b = 0;
            String output = rmsManager.createJsonLedRequest(a, a, ledColor);
            Serial2.print(output);
        }
    }
    else
    {
        Serial.println("Cannot Capture Vcell Data");
        LedColor ledColor;
        ledColor.r = 127;
        ledColor.g = 127;
        ledColor.b = 0;
        String output = rmsManager.createJsonLedRequest(a, a, ledColor);
        Serial2.print(output);
    }


    leds[led] = CRGB(129, 141, 214);

    FastLED.show();
    Serial.println("End Of Vcell");
    // Serial.print(".");
    // Serial.println("..............................////////////////...........");
}

void Temp(int id)
{
    int check = 0;
    int led = id - 1;
    int startIndex = id - 1; // because id start from 1, array index start from 0
    bool isAllDataCaptured = false;
    bool isAllDataNormal = false;
    Serial.println("Capturing Temperature Data");
    leds[led] = CRGB(227, 160, 17);
    FastLED.setBrightness(20);
    FastLED.show();
    String output = rmsManager.createJsonTempDataRequest(id);
    Serial2.print(output);
    DynamicJsonDocument docBattery(1024);
    deserializeJson(docBattery, Serial2);
    JsonObject object = docBattery.as<JsonObject>();
    
    if (!object.isNull())
    {
        if (docBattery.containsKey("TEMP"))
        {
            int arrSize = sizeof(docBattery["TEMP"]) / sizeof(docBattery["TEMP"][0]);
            if (arrSize >= 9)
            {
                for (int i = 0; i < 9; i++)
                {
                    float storage = docBattery["TEMP"][i];
                    temp[i] = (int32_t) storage * 1000;
                    cellData[startIndex].temp[i] = temp[i];
                }
                isAllDataCaptured = true;
            }
            
        }
            
    }
    
    int len = sizeof(temp) / sizeof(temp[0]);
    String t = arrToStr<int32_t>(temp, len);
    Serial.println("TEMP BID" + String(id));
    Serial.println(t);

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
            LedColor ledColor;
            ledColor.r = 0;
            ledColor.g = 200;
            ledColor.b = 0;
            String output = rmsManager.createJsonLedRequest(id, id, ledColor);
            Serial2.print(output);
        }
        else
        {
            Serial.println("Data Temperature Abnormal");
            LedColor ledColor;
            ledColor.r = 200;
            ledColor.g = 0;
            ledColor.b = 0;
            String output = rmsManager.createJsonLedRequest(id, id, ledColor);
            Serial2.print(output);
        }

        
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
        Serial.println("Cannot Capture Temperature Data");
        LedColor ledColor;
        ledColor.r = 127;
        ledColor.g = 127;
        ledColor.b = 0;
        String output = rmsManager.createJsonLedRequest(id, id, ledColor);
        Serial2.print(output);

    }

    leds[led] = CRGB(129, 141, 214);
    FastLED.show();
    delay(10);
}

void Vpack(int id)
{
    int check = 0;
    int led = id - 1;
    int startIndex = id - 1; // bid start from 1, array index start from 0
    bool isAllDataNormal = false;
    bool isAllDataCaptured = false;
    Serial.println("Capturing Vpack Data");
    leds[led] = CRGB(227, 160, 17);
    FastLED.setBrightness(20);
    FastLED.show();
    String output = rmsManager.createJsonVpackDataRequest(id);
    Serial2.print(output);
    DynamicJsonDocument docBattery(1024);
    deserializeJson(docBattery, Serial2);
    JsonObject object = docBattery.as<JsonObject>();
    
    if (!object.isNull())
    {
        if (docBattery.containsKey("VPACK"))
        {
            
            int arrSize = sizeof(docBattery["VPACK"]) / sizeof(docBattery["VPACK"][0]);
            
            if (arrSize >= 4)
            {
                for (int i = 0; i < 4; i++)
                {
                    vpack[i] = docBattery["VPACK"][i];
                    if (i != 0) // index 0 is for total vpack
                    {
                        cellData[startIndex].pack[i - 1] = vpack[i];
                    }
                }
                isAllDataCaptured = true;
            }
            
        }
    }
    int len = sizeof(vpack) / sizeof(vpack[0]);
    String t = arrToStr<int32_t>(vpack, len);
    Serial.println("VPACK" + String(id));
    Serial.println(t);

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
            LedColor ledColor;
            ledColor.r = 0;
            ledColor.g = 200;
            ledColor.b = 0;
            String output = rmsManager.createJsonLedRequest(id, id, ledColor);
            Serial2.print(output);
        }
        else
        {
            Serial.println("Vpack Data Abnormal");
            LedColor ledColor;
            ledColor.r = 200;
            ledColor.g = 0;
            ledColor.b = 0;
            String output = rmsManager.createJsonLedRequest(id, id, ledColor);
            Serial2.print(output);
        }
        
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
        Serial.println("Cannot Capture Vpack Data");
        LedColor ledColor;
        ledColor.r = 127;
        ledColor.g = 127;
        ledColor.b = 0;
        String output = rmsManager.createJsonLedRequest(id, id, ledColor);
        Serial2.print(output);
    }

    leds[led] = CRGB(129, 141, 214);
    FastLED.show();
    delay(10);
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
            String output = rmsManager.createShutDownRequest(i, a);
            Serial2.print(output);
            // serializeJson(docBattery, Serial2);
        }
    }
}

void getDeviceStatus(int id)
{
}

void performAddressing()
{
    DynamicJsonDocument docBattery(1024);
    for (int i = 0; i < numOfShiftRegister; i++)
    {
        int x = (8 * i) + 7; // 8 is number of shift register output
        int bid = i + 1; // id start from 1

        sr.set(x, HIGH);
        delay(1000);
        Serial.print("number = ");
        Serial.println(x);
        Serial.print("EHUB Number -> BMS === ");
        Serial.println(bid);
        docBattery["BID"] = bid;
        docBattery["SR"] = x;
        serializeJson(docBattery, Serial2);
        Serial.println("===============xxxxxxxxx===========");
        delay(100);
        sr.set(x, LOW);
        delay(100);
    }
    // if (i == (numOfShiftRegister - 1)) //if it is the last register (finished)
    // {
    //     docBattery["id"] = "Serial";
    //     serializeJson(docBattery, Serial2);
    //     Serial2.println("Hasil Respon = " + String(a));
    //     delay(500);
    //     //        goto menu2;
    //     goto menu2;
    // }
}

void performDataCollection()
{
    Serial.println("SEND DATA");
    leds[8] = CRGB(150, 120, 1);
    FastLED.show();
    delay(300);
    for (int i = 1; i <= 7; i++)
    {
        Temp(i);
        vcell(i);
        Vpack(i);
        getDeviceStatus(i);
    }
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

void setup()
{
    pinMode(relay[0], OUTPUT);
    pinMode(relay[1], OUTPUT);
    pinMode(buzzer, OUTPUT);
    Serial.begin(9600);
    Serial2.begin(115200);
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
    WiFi.hostname("esp32");
    WiFi.begin(ssid, password);

    digitalWrite(relay[0], LOW);
    digitalWrite(relay[1], LOW);
    // digitalWrite(buzzer, HIGH);
    // delay(500);
    // digitalWrite(buzzer, LOW);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial2.print(".");
        Serial.print(".");
        leds[9] = CRGB::Gold;
        FastLED.setBrightness(1);
        FastLED.show();
        delay(100);
        leds[9] = CRGB::GreenYellow;
        FastLED.setBrightness(20);
        FastLED.show();
        delay(100);
    }
    declareStruct();
    leds[9] = CRGB::LawnGreen;
    FastLED.setBrightness(20);
    FastLED.show();
    Serial2.println("wifi Connected");
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    declareStruct();
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
        size_t balancingCommandSize = sizeof(cellBalancingCommand) / sizeof(cellBalancingCommand[0]);
        for (size_t i = 0; i < balancingCommandSize; i++)
        {
        cellBalancingStatus[i].bid = cellBalancingCommand[i].bid;
        for (size_t j = 0; j < 45; j++)
        {
            cellBalancingStatus[i].cball[j] = cellBalancingCommand[i].cball[j];
        } 
        }
        response.replace(":status:", String(status));
        request->send(200, "application/json", response);}, 8192);

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
        int status = jsonManager.jsonAlarmCommandParser(input.c_str());
        alarmCommand.exec = status;
        commandStatus.alarmCommand = status;
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
        int status = jsonManager.jsonSleepCommandParser(input.c_str());
        sleepCommand.exec = status;
        commandStatus.sleepCommand = status;
        response.replace(":status:", String(status));
        request->send(200, "application/json", response); });

    server.addHandler(setBalancingHandler);
    server.addHandler(setAddressHandler);
    server.addHandler(setAlarmHandler);
    server.addHandler(setDataCollectionHandler);
    server.addHandler(setAlarmParamHandler);
    server.addHandler(setSleepHandler);

    // AsyncElegantOTA.begin(&server); // Start ElegantOTA
    server.begin();
    Serial.println("HTTP server started");
    delay(2000);
}

void loop()
{

    int qty;

    if (addressingCommand.exec)
    {
        // perform addressing
        Serial.println("Doing Addressing");
        performAddressing();
        addressingCommand.exec = 0;
        Serial.println("Addressing Finished");
    }
    else
    {
        if(dataCollectionCommand.exec)
        {
            //perform data collection
            if (address > 8)
            {
                address = 1;
            }
            Serial.println("Data Collection Address : " + String(address));
            // Temp(address);
            vcell(address);
            // Vpack(address);
            // address++;
            Serial.println("Data Collection Finished");
        }
        if(balancingCommand)
        {
            //perform balancing
        }
        if(alarmCommand.exec)
        {
            //perform alarm
        }
        if(sleepCommand.exec)
        {
            //perform sleep
            dataCollectionCommand.exec = 0;
            balancingCommand = 0;
            alarmCommand.exec = 0;
        }
    }

} // void loop end
