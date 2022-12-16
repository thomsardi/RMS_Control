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

String RMSManager::createCMSReadBalancingStatus(uint8_t bid)
{
    String output;
    DynamicJsonDocument docBattery(1024);
    docBattery["BID"] = bid;
    docBattery["RBAL"] = 1;
    serializeJson(docBattery, output);
    return output;
}

String RMSManager::createCMSStatusRequest(uint8_t bid)
{
    String output;
    DynamicJsonDocument docBattery(1024);
    docBattery["BID"] = bid;
    docBattery["RBQ"] = 1;
    serializeJson(docBattery, output);
    return output;
}

String RMSManager::createShutDownRequest(uint8_t bid)
{
    String output;
    DynamicJsonDocument docBattery(1024);
    docBattery["BID"] = bid;
    docBattery["SBQ"] = 1;
    serializeJson(docBattery, output);
    return output;
}

String RMSManager::createWakeupRequest(uint8_t bid)
{
    String output;
    DynamicJsonDocument docBattery(1024);
    docBattery["BID"] = bid;
    docBattery["WBQ"] = 1;
    serializeJson(docBattery, output);
    return output;
}

String RMSManager::createJsonLedRequest(const LedCommand &ledCommand)
{
    String output;
    DynamicJsonDocument doc(1024);
    doc["BID"] = ledCommand.bid;
    doc["LEDSET"] = ledCommand.ledset;
    doc["NUM_OF_LED"] = ledCommand.num_of_led;
    JsonArray led_rgb = doc.createNestedArray("LED_RGB");
    for (size_t i = 0; i < ledCommand.num_of_led; i++)
    {
        JsonArray led_rgb_0 = led_rgb.createNestedArray();
        led_rgb_0.add(ledCommand.red[i]);
        led_rgb_0.add(ledCommand.green[i]);
        led_rgb_0.add(ledCommand.blue[i]);
    }
    serializeJson(doc, output);
    return output;
}

String RMSManager::createCMSInfoRequest(uint8_t bid)
{
    String output;
    DynamicJsonDocument docBattery(1024);
    docBattery["BID"] = bid;
    docBattery["INFO"] = 1;
    serializeJson(docBattery, output);
    return output;
}

String RMSManager::createCMSResetRequest(uint8_t bid)
{
    String output;
    StaticJsonDocument<32> doc;
    doc["BID"] = bid;
    doc["RESTART"] = 1;
    serializeJson(doc, output);
    return output;
}

String RMSManager::createCMSFrameWriteIdRequest(uint8_t bid, String frameName)
{
    String output;
    StaticJsonDocument<96> doc;
    doc["BID"] = bid;
    doc["frame_write"] = 1;
    doc["frame_name"] = frameName;
    serializeJson(doc, output);
    return output;
}

String RMSManager::createCMSWriteBalancingRequest(uint8_t bid, const int cellCommand[], size_t numOfCellCommand)
{
    String output;
    DynamicJsonDocument doc(1024);
    doc["BID"] = bid;
    doc["SBAL"] = 1;
    JsonArray cball = doc.createNestedArray("cball");
    for (size_t i = 0; i < numOfCellCommand; i++)
    {
        cball.add(cellCommand[i]);
    }
    serializeJson(doc, output);
    return output;
}