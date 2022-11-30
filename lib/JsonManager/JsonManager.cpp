#include <JsonManager.h>

JsonManager::JsonManager()
{

}

String JsonManager::buildJsonData(const CellData cellData[], const size_t numOfJsonObject) 
{
    String result;
    DynamicJsonDocument doc(12288); //for 8 object
    JsonArray cms = doc.createNestedArray("cms_data");
    
    for (size_t i = 0; i < numOfJsonObject; i++)
    {
        JsonObject cms_0 = cms.createNestedObject();
        cms_0["frame_name"] = cellData[i].frameName;
        cms_0["bid"] = cellData[i].bid;
        JsonArray vcell = cms_0.createNestedArray("vcell");
        for ( int j = 0; j < 45; j++)
        {
            vcell.add(cellData[i].vcell[j]);
        }
        JsonArray temp = cms_0.createNestedArray("temp");
        for (int j = 0; j < 9; j++)
        {
            temp.add(cellData[i].temp[j]);
        }
        JsonArray vpack = cms_0.createNestedArray("pack");
        for (int k = 0; k < 3; k++)
        {
            vpack.add(cellData[i].pack[k]);
        }
        cms_0["sleep"] = cellData[i].status;
    }
    serializeJson(doc, result);
    return result;
}

String JsonManager::buildJsonRMSInfo(const RMSInfo& rmsInfo)
{
    String result;
    DynamicJsonDocument doc(256);
    doc["p_code"] = rmsInfo.p_code;
    doc["ver"] = rmsInfo.ver;
    doc["ip"] = rmsInfo.ip;
    doc["mac"] = rmsInfo.mac;
    doc["dev_type"] = rmsInfo.deviceTypeName;
    serializeJson(doc, result);
    return result;
}

String JsonManager::buildJsonCMSInfo(const CMSInfo cmsInfo[], size_t numOfJsonObject)
{
    String result;
    StaticJsonDocument<1024> doc;
    JsonArray cms_info = doc.createNestedArray("cms_info");
    for (size_t i = 0; i < numOfJsonObject; i++)
    {
        JsonObject cms_info_0 = cms_info.createNestedObject();
        cms_info_0["frame_name"] = cmsInfo[i].frameName;
        cms_info_0["bid"] = cmsInfo[i].bid;
        cms_info_0["p_code"] = cmsInfo[i].p_code;
        cms_info_0["ver"] = cmsInfo[i].ver;
        cms_info_0["chip"] = cmsInfo[i].chip;
    }
    serializeJson(doc, result);
    return result;
}

String JsonManager::buildJsonBalancingStatus(const CellBalancingStatus cellBalancingStatus[], size_t numOfJsonObject)
{
    String result;
    DynamicJsonDocument doc(8192);
    JsonArray balancing_status = doc.createNestedArray("balancing_status");
    
    for (size_t i = 0; i < numOfJsonObject; i++)
    {
        JsonObject balancing_status_0 = balancing_status.createNestedObject();
        balancing_status_0["bid"] = cellBalancingStatus[i].bid;
        JsonArray balancing_status_0_cball = balancing_status_0.createNestedArray("cball");
        for (size_t j = 0; j < 45; j++)
        {
            balancing_status_0_cball.add(cellBalancingStatus[i].cball[j]);
        }
    }
    serializeJson(doc, result);
    return result;
}

String JsonManager::buildJsonAlarmParameter(const AlarmParam& alarmParam)
{
    String result;
    StaticJsonDocument<64> doc;
    doc["vcell_max"] = alarmParam.vcell_max;
    doc["vcell_min"] = alarmParam.vcell_min;
    doc["temp_max"] = alarmParam.temp_max;
    doc["temp_min"] = alarmParam.temp_min;
    serializeJson(doc, result);
    return result;
}

String JsonManager::buildJsonCommandStatus(const CommandStatus& commandStatus)
{
    String result;
    StaticJsonDocument<64> doc;
    doc["addr"] = commandStatus.addrCommand;
    doc["alarm"] = commandStatus.alarmCommand;
    doc["data_collection"] = commandStatus.dataCollectionCommand;
    doc["sleep_command"] = commandStatus.sleepCommand;
    serializeJson(doc, result);
    return result;
}

int JsonManager::jsonBalancingCommandParser(const char* jsonInput, CellBalancingCommand cellBalancingCommand[])
{
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, jsonInput);
    if (error) 
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return -1;
    }
    JsonArray jsonArray = doc["balancing_command"].as<JsonArray>();
    if (jsonArray.isNull())
    {
        return -1;
    }
    for (JsonObject balancing_command_item : doc["balancing_command"].as<JsonArray>()) {
        int balancing_command_item_bid = balancing_command_item["bid"];
        int bidArrayIndex = balancing_command_item_bid - 1; // array start from index 0, while the bid start from 1
        cellBalancingCommand[bidArrayIndex].bid = balancing_command_item_bid;
        JsonArray balancing_command_item_cball = balancing_command_item["cball"];
        int index = 0;
        for (int bal : balancing_command_item_cball) {
            cellBalancingCommand[bidArrayIndex].cball[index] = bal;
            index++; 
        }
    }
    return 1;
}

int JsonManager::jsonAddressingCommandParser(const char* jsonInput)
{
    int command = 0;
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, jsonInput);
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return -1;
    }
    if (!doc.containsKey("addr")) 
    {
        return -1;
    }
    command = doc["addr"].as<signed int>();
    return command;
}

int JsonManager::jsonAlarmCommandParser(const char* jsonInput, AlarmCommand &alarmCommand)
{
    int status = -1;
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, jsonInput);
    if (error) 
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return -1;
    }

    JsonObject alarm = doc["alarm"];
    if (!alarm.isNull())
    {
        if (doc.containsKey("alarm"))
        {
            alarmCommand.buzzer = alarm["buzzer"]; // 1
            alarmCommand.powerRelay = alarm["power_relay"]; // 1
            alarmCommand.battRelay = alarm["batt_relay"]; // 1
            status = 1;
        }
        
    }
    
    return status;
}

int JsonManager::jsonDataCollectionCommandParser(const char* jsonInput)
{
    int command = 0;
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, jsonInput);
    if (error) 
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return -1;
    }

    if (!doc.containsKey("data_collection")) 
    {
        return -1;
    }

    command = doc["data_collection"].as<signed int>();
    return command;
}

int JsonManager::jsonAlarmParameterParser(const char* jsonInput, AlarmParam& alarmParam)
{
    StaticJsonDocument<128> doc;

    DeserializationError error = deserializeJson(doc, jsonInput);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return -1;
    }
    if (!doc.containsKey("vcell_max")) 
    {
        return -1;
    }
    alarmParam.vcell_max = doc["vcell_max"].as<signed int>();
    alarmParam.vcell_min = doc["vcell_min"].as<signed int>();
    alarmParam.temp_max = doc["temp_max"].as<signed int>();
    alarmParam.temp_min = doc["temp_min"].as<signed int>();
    return 1;
}

int JsonManager::jsonSleepCommandParser(const char* jsonInput)
{
    int command = 0;
    StaticJsonDocument<128> doc;

    DeserializationError error = deserializeJson(doc, jsonInput);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return -1;
    }

    if (!doc.containsKey("sleep")) 
    {
        return -1;
    }

    command = doc["sleep"].as<signed int>(); // 0
    return command;
}

int JsonManager::jsonCMSFrameParser(const char* jsonInput, FrameWrite &frameWrite)
{
    int command = 0;
    int bid = 0;
    int startIndex = 0;
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, jsonInput);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return -1;
    }

    if (doc.containsKey("bid"))
    {
        bid = doc["bid"];
        frameWrite.bid = bid;
        startIndex = bid -1;
    }
    else
    {
        return -1;
    }
    if (!doc.containsKey("frame_write")) 
    {
        return -1;
    }

    frameWrite.write = doc["frame_write"]; // 1
    if (frameWrite.write)
    {
        frameWrite.frameName = doc["frame_name"].as<String>();
        command = frameWrite.write;
    }
    return command;
}