#ifndef RMSMANAGER_H
#define RMSMANAGER_H

#include <Arduino.h>
#include <RMSManagerData.h>
#include <ArduinoJson.h>

class RMSManager {
    public :
        RMSManager();
        String createJsonVcellDataRequest(uint8_t bid);
        String createJsonTempDataRequest(uint8_t bid);
        String createJsonVpackDataRequest(uint8_t bid);
        String createJsonBalancingRequest(uint8_t bid);
        String createShutDownRequest(uint8_t bid, uint8_t bqNum);
        String createJsonLedRequest(uint8_t bid, uint8_t ledPosition, LedColor ledColor);
        String createCMSInfoRequest(uint8_t bid);
        // int sendVcellRequest(uint8_t bid);
        // int sendVpackRequest(uint8_t bid);
        // int sendTempRequest(uint8_t bid);
        // ReadResponseType readResponse(String input);
    private :

};



#endif