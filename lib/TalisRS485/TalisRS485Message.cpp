#include "TalisRS485Message.h"

TalisRS485Message::TalisRS485Message()
{
    
}

/**
 * Read request section
*/
bool TalisRS485Message::createVcellRequest(TalisRS485TxMessage &txMsg)
{
    String output;
    StaticJsonDocument<128> doc;
    doc["BID"] = txMsg.id;
    doc["VCELL"] = 1;
    serializeJson(doc, output);
    output += '\n';
    return txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());
}

bool TalisRS485Message::createTemperatureRequest(TalisRS485TxMessage &txMsg)
{
    String output;
    StaticJsonDocument<128> doc;
    doc["BID"] = txMsg.id;
    doc["TEMP"] = 1;
    serializeJson(doc, output);
    output += '\n';
    return txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());
}

bool TalisRS485Message::createVpackRequest(TalisRS485TxMessage &txMsg)
{
    String output;
    StaticJsonDocument<128> doc;
    doc["BID"] = txMsg.id;
    doc["VPACK"] = 1;
    serializeJson(doc, output);
    output += '\n';
    return txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());
}

bool TalisRS485Message::createCMSStatusRequest(TalisRS485TxMessage &txMsg)
{
    String output;
    StaticJsonDocument<128> doc;
    doc["BID"] = txMsg.id;
    doc["RBQ"] = 1;
    serializeJson(doc, output);
    output += '\n';
    return txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());

}

bool TalisRS485Message::createReadBalancingStatus(TalisRS485TxMessage &txMsg)
{
    String output;
    StaticJsonDocument<128> doc;
    doc["BID"] = txMsg.id;
    doc["RBAL"] = 1;
    serializeJson(doc, output);
    output += '\n';
    return txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());

}

bool TalisRS485Message::createShutDownRequest(TalisRS485TxMessage &txMsg)
{
    String output;
    StaticJsonDocument<128> doc;
    doc["BID"] = txMsg.id;
    doc["SBQ"] = 1;
    serializeJson(doc, output);
    output += '\n';
    return txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());
}

bool TalisRS485Message::createWakeupRequest(TalisRS485TxMessage &txMsg)
{
    String output;
    StaticJsonDocument<128> doc;
    doc["BID"] = txMsg.id;
    doc["WBQ"] = 1;
    serializeJson(doc, output);
    output += '\n';
    return txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());
}

bool TalisRS485Message::createCMSInfoRequest(TalisRS485TxMessage &txMsg)
{
    String output;
    StaticJsonDocument<128> doc;
    doc["BID"] = txMsg.id;
    doc["INFO"] = 1;
    serializeJson(doc, output);
    output += '\n';
    return txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());
}

bool TalisRS485Message::createCMSResetRequest(TalisRS485TxMessage &txMsg)
{
    String output;
    StaticJsonDocument<128> doc;
    doc["BID"] = txMsg.id;
    doc["RESTART"] = 1;
    serializeJson(doc, output);
    output += '\n';
    return txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());
}

/**
 * Write request section
*/
bool TalisRS485Message::createCMSFrameWriteIdRequest(TalisRS485TxMessage &txMsg, String frameName)
{
    String output;
    StaticJsonDocument<96> doc;
    doc["BID"] = txMsg.id;
    doc["frame_write"] = 1;
    doc["frame_name"] = frameName;
    serializeJson(doc, output);
    output += '\n';
    return txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());
}

bool createCMSCodeWriteRequest(TalisRS485TxMessage &txMsg, String cmsCode)
{
    String output;
    StaticJsonDocument<96> doc;
    doc["BID"] = txMsg.id;
    doc["cms_write"] = 1;
    doc["cms_code"] = cmsCode;
    serializeJson(doc, output);
    output += '\n';
    return txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());
}

bool TalisRS485Message::createCMSBaseCodeWriteRequest(TalisRS485TxMessage &txMsg, String baseCode)
{
    String output;
    StaticJsonDocument<96> doc;
    doc["BID"] = txMsg.id;
    doc["base_write"] = 1;
    doc["base_code"] = baseCode;
    serializeJson(doc, output);
    output += '\n';
    return txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());
}

bool TalisRS485Message::createCMSMcuCodeWriteRequest(TalisRS485TxMessage &txMsg, String mcuCode)
{
    String output;
    StaticJsonDocument<96> doc;
    doc["BID"] = txMsg.id;
    doc["mcu_write"] = 1;
    doc["mcu_code"] = mcuCode;
    serializeJson(doc, output);
    output += '\n';
    return txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());
}

bool TalisRS485Message::createCMSSiteLocationWriteRequest(TalisRS485TxMessage &txMsg, String siteLocation)
{
    String output;
    StaticJsonDocument<96> doc;
    doc["BID"] = txMsg.id;
    doc["site_write"] = 1;
    doc["site_location"] = siteLocation;
    serializeJson(doc, output);
    output += '\n';
    return txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());
}

bool TalisRS485Message::createCMSWriteBalancingRequest(TalisRS485TxMessage &txMsg, const int cellCommand[], size_t length)
{
    String output;
    StaticJsonDocument<768> doc;
    doc["BID"] = txMsg.id;
    doc["SBAL"] = 1;
    JsonArray cball = doc.createNestedArray("cball");
    for (size_t i = 0; i < length; i++)
    {
        cball.add(cellCommand[i]);
    }
    serializeJson(doc, output);
    output += '\n';
    return txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());
}

bool TalisRS485Message::createCMSWriteLedRequest(TalisRS485TxMessage &txMsg, const LedColor ledColor[], size_t length)
{
    String output;
    StaticJsonDocument<768> doc;
    doc["BID"] = txMsg.id;
    doc["LEDSET"] = 1;
    doc["NUM_OF_LED"] = length;
    JsonArray led_rgb = doc.createNestedArray("LED_RGB");
    for (size_t i = 0; i < length; i++)
    {
        JsonArray led_rgb_0 = led_rgb.createNestedArray();
        led_rgb_0.add(ledColor[i].red);
        led_rgb_0.add(ledColor[i].green);
        led_rgb_0.add(ledColor[i].blue);
    }
    serializeJson(doc, output);
    output += '\n';
    return txMsg.writeBuffer((uint8_t*)output.c_str(), output.length());
}