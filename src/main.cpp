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
#include <AsyncElegantOTA.h>
#include <HTTPClient.h>
#include <JsonManager.h>
#include <AsyncJson.h>

#define RXD2 16
#define TXD2 17
#define LED_PIN 27
#define NUM_LEDS 10
#define merah 202, 1, 9

int SET;
int cell[45];
float temp[9];
int vpack[4]; // index 0 is total vpack
int BACK;
int state = 0;
int emergency;
int reset = 0;
int alert = 0;
int relay[] = {22, 23};
int buzzer = 19;
// create a global shift register object
// parameters: <number of shift registers> (data pin, clock pin, latch pin)
ShiftRegister74HC595<8> sr(12, 14, 13);
DynamicJsonDocument docBattery(1024);
CRGB leds[NUM_LEDS];
const char *ssid = "RnD_Sundaya";
const char *password = "sundaya22";
const char *host = "192.168.2.174";

AsyncWebServer server(80);

JsonManager jsonManager;
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

void declareStruct()
{
  for (size_t i = 0; i < 8; i++)
  {
    cellData[i].bid = i+100;
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

  alarmParam.temp_max = 70;
  alarmParam.temp_min = 20;
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

String arrtostr(int a[], int len)
{
  String b;
  for (byte i = 0; i < len; i++)
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

String floatarrtostr(float a[], int len)
{
  String b;
  for (byte i = 0; i < len; i++)
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

  //  if (!client.connect(host, 443))
  //  {
  //    Serial2.println("Connection Failed");
  //    return;
  //  }
  int check = 0;
  int checknilairusak = 0;
  int checknilaisehat = 0;
  DynamicJsonDocument docBattery(1024);
  deserializeJson(docBattery, Serial2);
  BACK = docBattery["BACK"];

  int led = a - 1;
  int startIndex = a - 1; // bid start from 1, array index start from 0
  cellData[startIndex].bid = a;
  leds[led] = CRGB(227, 202, 9);
  FastLED.setBrightness(20);
  FastLED.show();
  if (state == 0)
  {
    DynamicJsonDocument docBattery(1024);
    docBattery["BID"] = a;
    docBattery["VCELL"] = 1;

    serializeJson(docBattery, Serial2);
    deserializeJson(docBattery, Serial2);
    JsonObject object = docBattery.as<JsonObject>();
    BACK = docBattery["BACK"];
    if (!object.isNull())
    {
      Serial2.print("masuk");
      for (int i = 0; i < 45; i++)
      {
        cell[i] = docBattery["VCELL"][i];
        cellData[startIndex].vcell[i] = cell[i];
      }
    }
    int len = sizeof(cell) / sizeof(cell[0]);
    String c = arrtostr(cell, len);

    Serial.println(c);

    WiFiClient client;
    client.connect(host, 443);

    for (int c : cell)
    {
      if (c >= 1)
      {
        check++;
      }
    }

    if (check == 45)
    {
      String Link;
      HTTPClient http;

      Link = "http://192.168.2.174/rakbatterybiru/src/logic/bms_vcell_update.php?cell=" + String(a) + "," + String(c);
      http.begin(client, Link);
      http.GET();
      //  String respon = http.getString();
      //  Serial.println(respon);
      http.end();
      Serial.println("berhasil");
      leds[led] = CRGB(78, 210, 219);
      FastLED.setBrightness(1);
      FastLED.show();
      delay(300);

      for (int c : cell)
      {
        if (c <= 200 || c >= 2500 || c <= 3800)
        {

          checknilaisehat++;
        }
        else
        {
          checknilairusak++;
        }
      }

      if (checknilaisehat == 44)
      {
        if (emergency >= 1)
        {
          emergency--;
        }

        //          {"BID": 1, "LEDSET": 1, "L": 1, "R": 112, "G": 121, "B": 145
        //          }
        DynamicJsonDocument docBattery(1024);
        docBattery["BID"] = a;
        docBattery["LEDSET"] = 1;
        docBattery["L"] = a;
        docBattery["R"] = 2;
        docBattery["G"] = 240;
        docBattery["B"] = 10;
        leds[9] = CRGB(2, 240, 9);
        FastLED.show();
        serializeJson(docBattery, Serial2);
        delay(100);
      }

      if (checknilairusak > 0)
      {
        emergency++;
        digitalWrite(buzzer, HIGH);
        delay(200);
        digitalWrite(buzzer, LOW);
        //          {"BID": 1, "LEDSET": 1, "L": 1, "R": 112, "G": 121, "B": 145
        //          }
        DynamicJsonDocument docBattery(1024);
        docBattery["BID"] = a;
        docBattery["LEDSET"] = 1;
        docBattery["L"] = a;
        docBattery["R"] = 200;
        docBattery["G"] = 1;
        docBattery["B"] = 1;
        leds[9] = CRGB(255, 10, 9);
        FastLED.show();
        serializeJson(docBattery, Serial2);
        delay(100);
      }

      leds[led] = CRGB(78, 227, 9);
      FastLED.setBrightness(20);
      FastLED.show();
      delay(300);

      if (emergency == 8)
      {
        Serial2.println("ERROR TONG");
        digitalWrite(buzzer, LOW);
        state = 1;
        alert = 1;
        BACK = 1;
        return;
      }
    }

    else
    {
      Serial.println("KOSONG");
      leds[led] = CRGB(227, 9, 34);

      FastLED.show();
      delay(1000);
    }

    if (BACK == 1)
    {
      Serial2.println("BACK");
      state = 1;
      leds[8] = CRGB(227, 9, 34);
      docBattery["BACKTOMENU"] = 1;
      serializeJsonPretty(docBattery, Serial2);
      FastLED.show();

      delay(100);
      return;
    }
  }

  if (BACK == 1)
  {
    Serial2.println("BACK");
    state = 1;
    leds[8] = CRGB(227, 9, 34);
    docBattery["BACKTOMENU"] = 1;
    serializeJsonPretty(docBattery, Serial2);
    FastLED.show();
    delay(100);
    return;
  }

  for (int i = 0; i < 45; i++)
  {
    cell[i] = 0;
    //      delay(10);
  }

  leds[led] = CRGB(129, 141, 214);

  FastLED.show();
  Serial.println("End Of Vcell");
  // Serial.print(".");
  // Serial.println("..............................////////////////...........");
}

void simulationvcell(int a)
{
  int led = a - 1;
  int startIndex = a - 1; // because BID start from 1, array index start from 0
  leds[led] = CRGB(227, 202, 9);
  FastLED.setBrightness(20);
  FastLED.show();
  DynamicJsonDocument docBattery(1024);
  docBattery["BID"] = a;
  docBattery["VCELL"] = 1;

  serializeJson(docBattery, Serial2);
  deserializeJson(docBattery, Serial2);
  JsonObject object = docBattery.as<JsonObject>();
  BACK = docBattery["BACK"];
  if (!object.isNull())
  {
    Serial2.print("masuk");
    for (int i = 0; i < 45; i++)
    {
      cell[i] = docBattery["VCELL"][i];
      // cellData[startIndex].vcell[i] = cell[i];
      delay(100);
    }
  }
  delay(100);
  int len = sizeof(cell) / sizeof(cell[0]);
  String c = arrtostr(cell, len);

  Serial.println(c);
  delay(10);
  WiFiClient client;
  client.connect(host, 443);

  if (cell[1] > 4 && cell[10] > 4 && cell[30] > 4)
  {
    Serial.println("berhasil");
    leds[led] = CRGB(78, 210, 219);
    FastLED.setBrightness(1);
    FastLED.show();
    delay(300);
    leds[led] = CRGB(78, 227, 9);
    FastLED.setBrightness(20);
    FastLED.show();
    delay(800);
  }
  if (BACK == 1)
  {
    Serial2.println("BACK");
    state = 1;
    leds[8] = CRGB(227, 9, 34);

    FastLED.show();
    delay(1000);
  }
  if (cell[1] < 4 && cell[10] < 4 && cell[30] < 4)
  {
    Serial.println("KOSONG");
    leds[led] = CRGB(227, 9, 34);

    FastLED.show();
    delay(1000);
  }
  leds[led] = CRGB(129, 141, 214);
  FastLED.show();
  Serial.print(".");
  Serial.println("..............................////////////////...........");
  delay(100);
}

void Temp(int id)
{
  int check = 0;
  DynamicJsonDocument docBattery(1024);
  deserializeJson(docBattery, Serial2);
  BACK = docBattery["BACK"];
  int led = id - 1;
  int startIndex = id - 1; // because id start from 1, array index start from 0

  if (state == 0)
  {

    leds[led] = CRGB(227, 160, 17);
    FastLED.setBrightness(20);
    FastLED.show();
    DynamicJsonDocument docBattery(1024);
    docBattery["BID"] = id;
    docBattery["TEMP"] = 1;

    serializeJson(docBattery, Serial2);
    deserializeJson(docBattery, Serial2);
    BACK = docBattery["BACK"];
    JsonObject object = docBattery.as<JsonObject>();

    if (!object.isNull())
    {
      Serial2.print("masuk");
      for (int i = 0; i < 9; i++)
      {
        temp[i] = docBattery["TEMP"][i];
        cellData[startIndex].temp[i] = temp[i];
        //        delay(10);
      }
      //      delay(10);
    }
    deserializeJson(docBattery, Serial2);
    BACK = docBattery["BACK"];
    int len = sizeof(temp) / sizeof(temp[0]);
    String t = floatarrtostr(temp, len);
    Serial.println("TEMP BID" + String(id));
    //    Serial.println("|  bq1t1|" + String (t1) + "|  bq1t2|" + String (t2) + "|  bq1t3|" + String (t3) + "|  bq2t1|" + String (t4) + "|  bq2t2|" + String (t5) + "|  bq2t3|" + String (t6) + "|  bq3t1|" + String (t7) + "| b3t2|" + String (t8) + "|  bq3t3|" + String (t9) + "||");
    Serial.println(t);
    //    delay (10);
    WiFiClient client;
    client.connect(host, 443);

    for (int tmp : temp)
    {
      //      if ( temp[1] > 4 &&  temp[2] > 4 &&  temp[8] > 4)
      if (tmp != 0)
      {
        check++;
      }
    }
    if (check > 4)
    {
      String Link;
      HTTPClient http;

      Link = "http://192.168.2.174/rakbatterybiru/src/logic/bms_temp_update.php?temp=" + String(id) + "," + String(t);
      http.begin(client, Link);
      http.GET();
      //        String respon = http.getString();
      //        Serial.println(respon);
      http.end();
      leds[led] = CRGB(10, 202, 9);
      FastLED.show();
      Serial.println("TEMP MASUK");
      //      delay(100);

      //      for (int tmp : temp) {
      //        if (tmp >= 100 || tmp <= 3) {
      //          emergency ++ ;
      //          digitalWrite(buzzer, HIGH);
      //          delay(100);
      //          digitalWrite(buzzer, LOW);
      //          //          {"BID": 1, "LEDSET": 1, "L": 1, "R": 112, "G": 121, "B": 145
      //          //          }
      //          DynamicJsonDocument docBattery(1024);
      //          docBattery["BID"] = id ;
      //          docBattery["LEDSET"] = 1;
      //          docBattery["L"] = id;
      //          docBattery["R"] = 220;
      //          docBattery["G"] = 60;
      //          docBattery["B"] = 1;
      //          leds[9] = CRGB(255, 1, 9);
      //          FastLED.show();
      //          serializeJson(docBattery, Serial2);
      //          delay(100);
      //        }
      //      }
      if (emergency == 8)
      {
        Serial2.println("ERROR TONG");
        digitalWrite(buzzer, LOW);
        state = 1;
        alert = 1;
        BACK = 1;
        return;
      }
    }

    else
    {
      Serial2.println("Gagal Masuk Temp");
      leds[led] = CRGB(merah);
      FastLED.show();
      delay(1000);
    }

    if (BACK == 1)
    {
      Serial2.println("BACK");
      state = 1;
      leds[8] = CRGB(227, 9, 34);
      docBattery["BACKTOMENU"] = 1;
      serializeJsonPretty(docBattery, Serial2);
      FastLED.show();
      delay(1000);
      return;
    }

    for (int i = 0; i < 9; i++)
    {
      temp[i] = 0;
      //      delay(10);
    }
  }
  if (BACK == 1)
  {
    Serial2.println("BACK");
    state = 1;
    leds[8] = CRGB(227, 9, 34);
    docBattery["BACKTOMENU"] = 1;
    serializeJsonPretty(docBattery, Serial2);
    FastLED.show();
    delay(1000);
    return;
  }
  leds[led] = CRGB(129, 141, 214);
  FastLED.show();
  delay(10);
}

void simulationTemp(int id)
{
  int check = 0;
  DynamicJsonDocument docBattery(1024);
  deserializeJson(docBattery, Serial2);
  BACK = docBattery["BACK"];
  int led = id - 1;
  int startIndex = id - 1; // bid start from 1, array index start from 0

  delay(10);

  if (state == 0)
  {

    leds[led] = CRGB(227, 160, 17);
    FastLED.setBrightness(20);
    FastLED.show();
    DynamicJsonDocument docBattery(1024);
    docBattery["BID"] = id;
    docBattery["TEMP"] = 1;

    serializeJson(docBattery, Serial2);
    deserializeJson(docBattery, Serial2);
    BACK = docBattery["BACK"];
    JsonObject object = docBattery.as<JsonObject>();

    if (!object.isNull())
    {
      Serial2.print("masuk");
      for (int i = 0; i < 9; i++)
      {
        temp[i] = docBattery["TEMP"][i];
        // cellData[startIndex].temp[i] = temp[i];
        delay(10);
      }
      delay(10);
    }
    deserializeJson(docBattery, Serial2);
    BACK = docBattery["BACK"];
    int len = sizeof(temp) / sizeof(temp[0]);
    String t = floatarrtostr(temp, len);
    Serial.println("TEMP BID" + String(id));
    //    Serial.println("|  bq1t1|" + String (t1) + "|  bq1t2|" + String (t2) + "|  bq1t3|" + String (t3) + "|  bq2t1|" + String (t4) + "|  bq2t2|" + String (t5) + "|  bq2t3|" + String (t6) + "|  bq3t1|" + String (t7) + "| b3t2|" + String (t8) + "|  bq3t3|" + String (t9) + "||");
    Serial.println(t);
    delay(10);

    for (int tmp : temp)
    {
      //      if ( temp[1] > 4 &&  temp[2] > 4 &&  temp[8] > 4)
      if (tmp >= 1)
      {
        check++;
      }
    }
    if (check == 9)
    {
      leds[led] = CRGB(10, 202, 9);
      FastLED.show();
      Serial.println("TEMP MASUK");
      delay(100);
    }

    else
    {
      Serial2.println("Gagal Masuk Temp");
      leds[led] = CRGB(merah);
      FastLED.show();
      delay(1000);
    }

    //  for (int tmp : temp) {
    //    if ( tmp <= 1) {
    //      Serial2.println("Gagal Masuk Temp");
    //      leds[led] = CRGB(merah);
    //      FastLED.show();
    //      delay(1000);
    //      break ;
    //    }
    //  }

    if (BACK == 1)
    {
      Serial2.println("BACK");
      state = 1;
      leds[8] = CRGB(227, 9, 34);
      docBattery["BACKTOMENU"] = 1;
      serializeJsonPretty(docBattery, Serial2);
      FastLED.show();
      delay(1000);
    }

    for (int i = 0; i < 9; i++)
    {
      temp[i] = 0;
      delay(10);
    }
  }
  if (BACK == 1)
  {
    Serial2.println("BACK");
    state = 1;
    leds[8] = CRGB(227, 9, 34);
    docBattery["BACKTOMENU"] = 1;
    serializeJsonPretty(docBattery, Serial2);
    FastLED.show();
    delay(1000);
  }

  leds[led] = CRGB(129, 141, 214);
  FastLED.show();
  delay(10);
}

void simulationVpack(int id)
{
  int check = 0;
  DynamicJsonDocument docBattery(1024);
  deserializeJson(docBattery, Serial2);
  BACK = docBattery["BACK"];
  int led = id - 1;
  int startIndex = id - 1; // bid start from 1, array index start from 0

  delay(10);

  if (state == 0)
  {

    leds[led] = CRGB(227, 160, 17);
    FastLED.setBrightness(20);
    FastLED.show();
    DynamicJsonDocument docBattery(1024);
    docBattery["BID"] = id;
    docBattery["VPACK"] = 1;

    serializeJson(docBattery, Serial2);
    deserializeJson(docBattery, Serial2);
    BACK = docBattery["BACK"];
    JsonObject object = docBattery.as<JsonObject>();

    if (!object.isNull())
    {
      Serial2.print("masuk");
      for (int i = 0; i < 4; i++)
      {
        vpack[i] = docBattery["VPACK"][i];
        // cellData[startIndex].pack[i] = vpack[i];
        delay(10);
      }
    }
    delay(10);
    deserializeJson(docBattery, Serial2);
    BACK = docBattery["BACK"];
    int len = sizeof(vpack) / sizeof(vpack[0]);
    String t = arrtostr(vpack, len);
    Serial.println("VPACK" + String(id));
    //    //    Serial.println("|  bq1t1|" + String (t1) + "|  bq1t2|" + String (t2) + "|  bq1t3|" + String (t3) + "|  bq2t1|" + String (t4) + "|  bq2t2|" + String (t5) + "|  bq2t3|" + String (t6) + "|  bq3t1|" + String (t7) + "| b3t2|" + String (t8) + "|  bq3t3|" + String (t9) + "||");
    Serial.println(t);
    delay(10);

    for (int vpck : vpack)
    {
      //      if ( temp[1] > 4 &&  temp[2] > 4 &&  temp[8] > 4)
      if (vpck >= 1)
      {
        check++;
      }
    }

    if (check == 4)
    {
      leds[led] = CRGB(10, 202, 9);
      FastLED.show();
      Serial.println("VPACK MASUK");
      delay(100);
    }

    else
    {
      Serial2.println("Gagal Masuk VPACK");
      leds[led] = CRGB(merah);
      FastLED.show();
      delay(1000);
    }

    //  for (int tmp : temp) {
    //    if ( tmp <= 1) {
    //      Serial2.println("Gagal Masuk Temp");
    //      leds[led] = CRGB(merah);
    //      FastLED.show();
    //      delay(1000);
    //      break ;
    //    }
    //  }

    if (BACK == 1)
    {
      Serial2.println("BACK");
      state = 1;
      leds[8] = CRGB(227, 9, 34);
      docBattery["BACKTOMENU"] = 1;
      serializeJsonPretty(docBattery, Serial2);
      FastLED.show();
      delay(1000);
    }
    //
    for (int i = 0; i < 4; i++)
    {
      vpack[i] = 0;
      delay(10);
    }
  }
  if (BACK == 1)
  {
    Serial2.println("BACK");
    state = 1;
    leds[8] = CRGB(227, 9, 34);
    docBattery["BACKTOMENU"] = 1;
    serializeJsonPretty(docBattery, Serial2);
    FastLED.show();
    delay(1000);
  }

  leds[led] = CRGB(129, 141, 214);
  FastLED.show();
  delay(10);
}

void Vpack(int id)
{
  int check = 0;
  DynamicJsonDocument docBattery(1024);
  deserializeJson(docBattery, Serial2);
  BACK = docBattery["BACK"];
  int led = id - 1;
  int startIndex = id - 1; // bid start from 1, array index start from 0

  //  delay(10);

  if (state == 0)
  {

    leds[led] = CRGB(227, 160, 17);
    FastLED.setBrightness(20);
    FastLED.show();
    DynamicJsonDocument docBattery(1024);
    docBattery["BID"] = id;
    docBattery["VPACK"] = 1;

    serializeJson(docBattery, Serial2);
    deserializeJson(docBattery, Serial2);
    BACK = docBattery["BACK"];
    JsonObject object = docBattery.as<JsonObject>();

    if (!object.isNull())
    {
      Serial2.print("masuk");
      for (int i = 0; i < 4; i++)
      {
        vpack[i] = docBattery["VPACK"][i];
        if (i != 0) // index 0 is for total vpack
        {
          cellData[startIndex].pack[i - 1] = vpack[i];
        }
        //        delay(10);
      }
    }
    //    delay (10);
    deserializeJson(docBattery, Serial2);
    BACK = docBattery["BACK"];
    int len = sizeof(vpack) / sizeof(vpack[0]);
    String t = arrtostr(vpack, len);
    Serial.println("VPACK" + String(id));
    //    //    Serial.println("|  bq1t1|" + String (t1) + "|  bq1t2|" + String (t2) + "|  bq1t3|" + String (t3) + "|  bq2t1|" + String (t4) + "|  bq2t2|" + String (t5) + "|  bq2t3|" + String (t6) + "|  bq3t1|" + String (t7) + "| b3t2|" + String (t8) + "|  bq3t3|" + String (t9) + "||");
    Serial.println(t);
    WiFiClient client;
    client.connect(host, 443);
    //    delay (10);

    for (int vpck : vpack)
    {
      //      if ( temp[1] > 4 &&  temp[2] > 4 &&  temp[8] > 4)
      if (vpck >= 1)
      {
        check++;
      }
    }

    if (check == 4)
    {
      String Link;
      HTTPClient http;

      Link = "http://192.168.2.174/rakbatterybiru/src/logic/bms_vpack_update.php?vpack=" + String(id) + "," + String(t);
      http.begin(client, Link);
      http.GET();
      //        String respon = http.getString();
      //        Serial.println(respon);
      http.end();
      leds[led] = CRGB(10, 202, 9);
      FastLED.show();
      Serial.println("VPACK MASUK");
    }

    else
    {
      Serial2.println("Gagal Masuk VPACK");
      leds[led] = CRGB(merah);
      FastLED.show();
      delay(1000);
    }

    //  for (int tmp : temp) {
    //    if ( tmp <= 1) {
    //      Serial2.println("Gagal Masuk Temp");
    //      leds[led] = CRGB(merah);
    //      FastLED.show();
    //      delay(1000);
    //      break ;
    //    }
    //  }

    if (BACK == 1)
    {
      Serial2.println("BACK");
      state = 1;
      leds[8] = CRGB(227, 9, 34);
      docBattery["BACKTOMENU"] = 1;
      serializeJsonPretty(docBattery, Serial2);
      FastLED.show();
      delay(1000);
    }
    //
    for (int i = 0; i < 4; i++)
    {
      vpack[i] = 0;
      //      delay(10);
    }
  }
  if (BACK == 1)
  {
    Serial2.println("BACK");
    state = 1;
    leds[8] = CRGB(227, 9, 34);
    docBattery["BACKTOMENU"] = 1;
    serializeJsonPretty(docBattery, Serial2);
    FastLED.show();
    delay(1000);
  }

  leds[led] = CRGB(129, 141, 214);
  FastLED.show();
  delay(10);
  Serial.println("End Of Vpack");
}

void getDeviceStatus(int id)
{
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
  digitalWrite(buzzer, HIGH);
  delay(500);
  digitalWrite(buzzer, LOW);
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
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Hi! I am ESP32."); });

  server.on("/get-cms-data", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    size_t cellDataArrSize = sizeof(cellData) / sizeof(cellData[0]);
    // Serial.println(cellDataArrSize);
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

  AsyncCallbackJsonWebHandler *setBalancingHandler = new AsyncCallbackJsonWebHandler(
      "/set-balancing", [](AsyncWebServerRequest *request, JsonVariant &json)
      {
    // JsonObject jsonObj = json.as<JsonObject>();
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
    request->send(200, "application/json", response); },
      8192);

  AsyncCallbackJsonWebHandler *setAddressHandler = new AsyncCallbackJsonWebHandler("/set-addressing", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                                   {
    // JsonObject jsonObj = json.as<JsonObject>();
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
    // JsonObject jsonObj = json.as<JsonObject>();
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
    // JsonObject jsonObj = json.as<JsonObject>();
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
    // JsonObject jsonObj = json.as<JsonObject>();
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
    // JsonObject jsonObj = json.as<JsonObject>();
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

  AsyncElegantOTA.begin(&server); // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
  delay(2000);
}

void loop()
{

  int qty;

menu1:
  while (1)
  {
    DynamicJsonDocument docBattery(1024);
    int i;
    //    int respon;
    int a = 0;
    for (i = 0; i < 8; i++)
    {
      int x = (8 * i) + 7;
      int b = i + 1;

      sr.set(x, HIGH);
      delay(1000);
      Serial.print("no  =");
      Serial.println(x);
      Serial.print("NO EHUB -> BMS ===");
      Serial.println(b);
      docBattery["BID"] = b;
      docBattery["SR"] = x;
      serializeJson(docBattery, Serial2);
      Serial.println("===============xxxxxxxxx===========");
      delay(100);
      sr.set(x, LOW);
      delay(100);
      if (i == 7)

      {
        docBattery["id"] = "Serial";
        serializeJson(docBattery, Serial2);
        Serial2.println("Hasil Respon = " + String(a));
        delay(500);
        //        goto menu2;
        goto menu2;
      }
    }
  }

menu2:
  while (1)
  {

    state = 0;
    leds[8] = CRGB(1, 240, 1);
    FastLED.setBrightness(20);
    FastLED.show();
    DynamicJsonDocument docBattery(1024);
    deserializeJson(docBattery, Serial2);
    int GET = docBattery["GET"];
    int SEND = docBattery["SEND"];
    int PANIC = docBattery["PANIC"];
    int PUSH = docBattery["PUSH"];
    qty = docBattery["QTY"];
    SET = docBattery["SET"];
    BACK = docBattery["BACK"];

    char jsonparse[500];
  http: // 192.168.2.174/rakbatterybiru/src/logic/bms_vcell_update.php?c
    String getconfig = parseconfig("http://192.168.2.174/rakbatterybiru/src/logic/configrms.json");
    getconfig.toCharArray(jsonparse, 500);
    deserializeJson(docBattery, jsonparse);
    SEND = docBattery["SEND"];
    qty = docBattery["QTY"];

    Serial.println("SEND :" + String(SEND));
    Serial.println("qty :" + String(qty));

    if (SET == 1)
    {
      Serial2.println("SET");
      // Serial.println("SET");
      leds[8] = CRGB(150, 120, 1);
      FastLED.show();
      goto menu1;
      delay(300);
    }

    if (GET == 1)
    {
      Serial2.println("GET");
      // Serial.println("GET");
      leds[8] = CRGB(150, 120, 1);
      FastLED.show();
      goto getdata;
      delay(300);
    }

    if (SEND == 1)
    {
      Serial2.println("SEND");
      // Serial1.println("SEND");
      leds[8] = CRGB(150, 120, 1);
      FastLED.show();
      goto senddata;
      delay(300);
    }

    if (PANIC == 1 || alert == 1)
    {
      Serial2.println("PANIC");
      // Serial1.println("PANIC");
      goto buzzer;
    }

    if (PUSH > 0)
    {
      delay(100);
      DynamicJsonDocument docBattery(1024);
      Serial2.println("PUSH DATA SEBANYAK" + String(PUSH));
      digitalWrite(buzzer, HIGH);
      delay(200);
      digitalWrite(buzzer, LOW);
      delay(100);
      digitalWrite(buzzer, HIGH);
      delay(100);
      digitalWrite(buzzer, LOW);
      leds[8] = CRGB(1, 100, 200);
      FastLED.setBrightness(20);

      for (int i = 0; i <= 7; i++)
      {
        docBattery["BID"] = i + 1;
        docBattery["LEDSET"] = 1;
        docBattery["L"] = i;
        docBattery["R"] = 190;
        docBattery["G"] = 145;
        docBattery["B"] = 10;
        serializeJson(docBattery, Serial2);
        delay(300);
      }
      deserializeJson(docBattery, Serial2);
      delay(100);
      for (int i = 1; i <= PUSH; i++)
      {
        if (state == 1)
        {
          break;
        }
        for (int i = 1; i <= 7; i++)
        {
          if (state == 1)
          {
            break;
          }
          Temp(i);
          //      delay(500);
          vcell(i);
          //      delay(1000);
          Vpack(i);
          //      delay(500);
        }
      }
      PUSH = 0;

      for (int i = 0; i <= 7; i++)
      {
        docBattery["BID"] = i + 1;
        docBattery["LEDSET"] = 1;
        docBattery["L"] = i;
        docBattery["R"] = 32;
        docBattery["G"] = 178;
        docBattery["B"] = 170;
        serializeJson(docBattery, Serial2);
        delay(100);
      }
      digitalWrite(buzzer, HIGH);
      delay(500);
      digitalWrite(buzzer, LOW);
      delay(100);
      digitalWrite(buzzer, HIGH);
      delay(100);
      digitalWrite(buzzer, LOW);
    }

    //    delay(500);
  }

buzzer:
  while (1)
  {
    if (reset == 0)
    {

      for (int i = 1; i <= 8; i++)
      {
        for (int a = 1; a <= 3; a++)
        {
          docBattery["BID"] = i;
          docBattery["SBQ"] = a;
          DynamicJsonDocument docBattery(768);
          serializeJson(docBattery, Serial2);
        }
      }
      digitalWrite(relay[0], HIGH);
      digitalWrite(relay[1], HIGH);
      digitalWrite(buzzer, HIGH);
      delay(500);
      digitalWrite(relay[1], LOW);
      digitalWrite(buzzer, HIGH);
      delay(500);
      digitalWrite(relay[1], HIGH);
      digitalWrite(buzzer, HIGH);
      delay(500);
    }

    for (int i = 0; i < 8; i++)
    {
      leds[i] = CRGB::Red;
      FastLED.show();
      Serial2.println("Sedang Emergency");
      digitalWrite(buzzer, LOW);
      delay(500);
      digitalWrite(buzzer, HIGH);
      delay(500);
      leds[i] = CRGB::Yellow;
      FastLED.show();
      reset = i + 1;
    }
  }

getdata:
  while (1)
  {
    DynamicJsonDocument docBattery(1024);
    deserializeJson(docBattery, Serial2);
    BACK = docBattery["BACK"];

    if (BACK == 1 || state == 1)
    {
      docBattery["BACK"] = 1;
      DynamicJsonDocument docBattery(768);
      serializeJson(docBattery, Serial2);
      goto menu2;
      delay(100);
    }

    for (int i = 1; i < 3; i++)
    {
      //      simulationTemp(i);
      delay(500);
      simulationvcell(i);
      delay(1000);
      simulationVpack(i);
    }

    //    simulationVpack(1);
    //    delay(200);
  }

senddata:
  while (1)
  {

    DynamicJsonDocument docBattery(1024);
    deserializeJson(docBattery, Serial2);
    BACK = docBattery["BACK"];

    if (BACK == 1 || state == 1)
    {
      Serial2.println("BACK");
      goto menu2;
      delay(100);
    }

    for (int i = 0; i < qty; i++)
    {
      for (int i = 1; i <= 7; i++)
      {
        if (state == 1)
        {
          break;
        }
        Temp(i);
        //      delay(500);
        vcell(i);
        //      delay(1000);
        Vpack(i);
        //      delay(500);
        getDeviceStatus(i);
      }
      //    simulationVpack(1);
      //    delay(1000);
    }
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    delay(100);
    digitalWrite(buzzer, HIGH);
    delay(100);
    digitalWrite(buzzer, LOW);
    goto menu2;
  }

} // void loop end
