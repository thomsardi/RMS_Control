#include <RMSManager.h>

RMSManager::RMSManager()
{

}

String RMSManager::createJsonVcellDataRequest(uint8_t bid)
{
    String output;
    DynamicJsonDocument docBattery(1024);
    docBattery["BID"] = bid;
    docBattery["VCELL"] = 1;
    serializeJson(docBattery, output);
    return output;
}

String RMSManager::createJsonTempDataRequest(uint8_t bid)
{
    String output;
    DynamicJsonDocument docBattery(1024);
    docBattery["BID"] = bid;
    docBattery["TEMP"] = 1;
    serializeJson(docBattery, output);
    return output;
}

String RMSManager::createJsonVpackDataRequest(uint8_t bid)
{
    String output;
    DynamicJsonDocument docBattery(1024);
    docBattery["BID"] = bid;
    docBattery["VPACK"] = 1;
    serializeJson(docBattery, output);
    return output;
}

String RMSManager::createShutDownRequest(uint8_t bid, uint8_t bqNum)
{
    String output;
    DynamicJsonDocument docBattery(1024);
    docBattery["BID"] = bid;
    docBattery["SBQ"] = bqNum;
    serializeJson(docBattery, output);
    return output;
}

String RMSManager::createJsonLedRequest(uint8_t bid, uint8_t ledPosition, LedColor ledColor)
{
    String output;
    DynamicJsonDocument docBattery(1024);
    docBattery["BID"] = bid;
    docBattery["LEDSET"] = 1;
    docBattery["L"] = ledPosition;
    docBattery["R"] = ledColor.r;
    docBattery["G"] = ledColor.g;
    docBattery["B"] = ledColor.b;
    serializeJson(docBattery, output);
    return output;
}