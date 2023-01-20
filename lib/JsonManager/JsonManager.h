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

#include <ArduinoJson.h>
#include <JsonData.h>

class JsonManager {
    public :
        JsonManager();
        //GET method (read the data)
        String buildSingleJsonData(const CellData &cellData);
        String buildJsonData(const CellData cellData[], const size_t numOfJsonObject = 8); // get 8 cms data
        String buildJsonRMSInfo(const RMSInfo& rmsInfo); // get rms info
        String buildJsonCMSInfo(const CMSInfo cmsInfo[], size_t numOfJsonObject = 8); // get 8 cms info
        String buildJsonBalancingStatus(const CellBalancingStatus cellBalancingStatus[], size_t numOfJsonObject); //get 8 cms balancing status
        String buildJsonAlarmParameter(const AlarmParam& alarmParam); // get RMS alarm parameter
        String buildJsonCommandStatus(const CommandStatus& commandStatus); // get RMS command status (addressing, alarm, data capture, sleep)
        String buildJsonAddressingStatus(const AddressingStatus &addressingStatus, size_t arraySize);

        // POST method (write the data)
        int jsonBalancingCommandParser(const char* jsonInput, CellBalancingCommand &cellBalancingCommand); // set balancing command for each cell
        int jsonAddressingCommandParser(const char* jsonInput); // set addressing command
        int jsonAlarmCommandParser(const char* jsonInput, AlarmCommand &alarmCommand); // set alarm command
        int jsonDataCollectionCommandParser(const char* jsonInput); // set data collection command
        int jsonAlarmParameterParser(const char* jsonInput, AlarmParam& alarmParam); // set alarm parameter
        int jsonSleepCommandParser(const char* jsonInput); // set sleep command
        int jsonCMSFrameParser(const char* jsonInput, FrameWrite &frameWrite);
        int jsonBalancingStatusParser(const char* jsonInput, CellBalancingStatus cellBalancingStatus[]);
        int jsonCMSShutdownParser(const char* jsonInput, CMSShutDown &cmsShutdown);
        int jsonCMSWakeupParser(const char* jsonInput, CMSWakeup &cmsWakeup);
        int jsonLedParser(const char* jsonInput, LedCommand &ledCommand);
        int jsonCMSRestartParser(const char* jsonInput, CMSRestartCommand &cmsRestartCommand);
        int jsonCMSRestartPinParser(const char* jsonInput);
        int jsonRMSRestartParser(const char* jsonInput);

        private :
        int getBit(int pos, int data);
};


#endif