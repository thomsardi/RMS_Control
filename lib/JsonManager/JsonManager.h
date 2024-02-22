#ifndef JSONMANAGER_H
#define JSONMANAGER_H

#define JSONMANAGER_DEBUG

#ifdef JSONMANAGER_DEBUG
    #define LOG_PRINT(x)    Serial.print(x)
    #define LOG_PRINTLN(x)  Serial.println(x)
#else
    #define LOG_PRINT(x)
    #define LOG_PRINTLN(x)
#endif

#include <Arduino.h>
#include <ArduinoJson.h>
#include <JsonData.h>
#include <ESPAsyncWebServer.h>
#include <Vector.h>

struct NetworkSetting {
    String ssid;
    String pass;
    String ip;
    String gateway;
    String subnet;
    int8_t server; //server = 1 is for static, 2 for dhcp
    int8_t mode; //mode = 1 is for AP, 2 for Station
    bool flag = 0;
};

class JsonManager {
    public :
        JsonManager();
        //GET method (read the data)
        int processSingleCmsDataRequest(AsyncWebServerRequest *request); //extract get parameter, return bid index
        String buildSingleJsonData(const CMSData &cmsData);
        int buildJsonData(AsyncWebServerRequest *request, const PackedData &packedData, String &buffer); //get variable cms data
        String buildJsonData(AsyncWebServerRequest *request, const PackedData &packedData); // get 8 cms data
        String buildJsonRMSInfo(const RMSInfo& rmsInfo); // get rms info
        String buildJsonBalancingStatus(const std::map<int, CellBalanceState> &cellBalanceState); //get 8 cms balancing status
        String buildJsonAlarmParameter(const AlarmParam& alarmParam); // get RMS alarm parameter
        String buildJsonCommandStatus(const CommandStatus& commandStatus); // get RMS command status (addressing, alarm, data capture, sleep)
        String buildJsonAddressingStatus(const AddressingStatus &addressingStatus, size_t arraySize);
        String getNetworkInfo(const NetworkSetting &networkSetting);
        String getUserNetworkSetting(const NetworkSetting &networkSetting);
        String getUserAlarmSetting(const AlarmParam &alarmParam);
        void parser(const String &input, char delimiter, Vector<String> &valueVec);
        

        // POST method (write the data)
        int jsonBalancingCommandParser(const char* jsonInput, CellBalancingCommand &cellBalancingCommand); // set balancing command for each cell
        int jsonAddressingCommandParser(const char* jsonInput); // set addressing command
        int jsonAlarmCommandParser(const char* jsonInput, AlarmCommand &alarmCommand); // set alarm command
        int jsonDataCollectionCommandParser(const char* jsonInput); // set data collection command
        int jsonAlarmParameterParser(const char* jsonInput, AlarmParam& alarmParam); // set alarm parameter
        int jsonHardwareAlarmEnableParser(const char* jsonInput, bool &hardwareAlarmEnable); // enable /disable alarm
        int jsonSleepCommandParser(const char* jsonInput); // set sleep command
        int jsonRmsCodeParser(const char* jsonInput, MasterWrite &masterWrite);
        int jsonRmsRackSnParser(const char* jsonInput, MasterWrite &masterWrite);
        int jsonCMSFrameParser(const char* jsonInput, SlaveWrite &slaveWrite);
        int jsonCMSCodeParser(const char* jsonInput, SlaveWrite &slaveWrite);
        int jsonCMSBaseCodeParser(const char* jsonInput, SlaveWrite &slaveWrite);
        int jsonCMSMcuCodeParser(const char* jsonInput, SlaveWrite &slaveWrite);
        int jsonCMSSiteLocationParser(const char* jsonInput, SlaveWrite &slaveWrite);
        int jsonCMSShutdownParser(const char* jsonInput, CMSShutDown &cmsShutdown);
        int jsonCMSWakeupParser(const char* jsonInput, CMSWakeup &cmsWakeup);
        int jsonLedParser(const char* jsonInput, LedCommand &ledCommand);
        int jsonCMSRestartParser(const char* jsonInput, CMSRestartCommand &cmsRestartCommand);
        int jsonCMSRestartPinParser(const char* jsonInput);
        int jsonRMSRestartParser(const char* jsonInput);
        NetworkSetting parseNetworkSetting(JsonVariant &json);
        int parseFactoryReset(JsonVariant &json);

    private :
        int getBit(int pos, int data);
        bool isNumber(const String &input);
};


#endif