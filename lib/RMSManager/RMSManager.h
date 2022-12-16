#ifndef RMSMANAGER_H
#define RMSMANAGER_H

#include <Arduino.h>
#include <RMSManagerData.h>
#include <ArduinoJson.h>
#include <JsonManager.h>

class RMSManager {
    public :
        RMSManager();
        String createJsonVcellDataRequest(uint8_t bid);
        String createJsonTempDataRequest(uint8_t bid);
        String createJsonVpackDataRequest(uint8_t bid);
        String createCMSWriteBalancingRequest(uint8_t bid, const int cellCommand[], size_t numOfCellCommand = 45);
        String createJsonReadBalancingRequest(uint8_t bid);
        String createShutDownRequest(uint8_t bid);
        String createWakeupRequest(uint8_t bid);
        String createCMSStatusRequest(uint8_t bid);
        String createJsonLedRequest(const LedCommand &ledCommand);
        String createCMSInfoRequest(uint8_t bid);
        String createCMSFrameWriteIdRequest(uint8_t bid, String frameId);
        String createCMSReadBalancingStatus(uint8_t bid);
        String createCMSResetRequest(uint8_t bid);
        // int sendVcellRequest(uint8_t bid);
        // int sendVpackRequest(uint8_t bid);
        // int sendTempRequest(uint8_t bid);
        // ReadResponseType readResponse(String input);
    private :

};



#endif