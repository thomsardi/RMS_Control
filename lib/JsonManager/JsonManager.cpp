#include <JsonManager.h>

JsonManager::JsonManager()
{

}

String JsonManager::buildSingleJsonData(const CellData &cellData)
{
    String result;
    StaticJsonDocument<1536> doc; // for 8 object
    doc["frame_name"] = cellData.frameName;
    doc["bid"] = cellData.bid;
    JsonArray vcell = doc.createNestedArray("vcell");
    for (int j = 0; j < 45; j++)
    {
        vcell.add(cellData.vcell[j]);
    }
    JsonArray temp = doc.createNestedArray("temp");
    for (int j = 0; j < 9; j++)
    {
        temp.add(cellData.temp[j]);
    }
    JsonArray vpack = doc.createNestedArray("pack");
    for (int k = 0; k < 3; k++)
    {
        vpack.add(cellData.pack[k]);
    }
    doc["wake_status"] = cellData.status;
    serializeJson(doc, result);
    return result;
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
        cms_0["wake_status"] = cellData[i].status;
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

String JsonManager::buildJsonAddressingStatus(const AddressingStatus &addressingStatus, size_t arraySize)
{
    String result;
    StaticJsonDocument<256> doc;
    doc["num_of_device"] = arraySize;
    JsonArray device_address_list = doc.createNestedArray("device_address_list");
    if (arraySize != 0)
    {
        for(int i = 0; i < arraySize; i++)
        {
            device_address_list.add(addressingStatus.deviceAddressList[i]);
        }
    }
    else
    {
        device_address_list.add(0);
    }
    doc["status"] = addressingStatus.status;
    serializeJson(doc, result);
    return result;
}

int JsonManager::jsonBalancingCommandParser(const char* jsonInput, CellBalancingCommand &cellBalancingCommand)
{
    int status = 0;
    int bid = 0;
    int startIndex = 0;
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, jsonInput);
    if (error) 
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return -1;
    }

    JsonObject balancing_command = doc["balancing_command"];
    
    if (balancing_command.isNull())
    {
        return -1;
    }

    if(balancing_command.containsKey("bid"))
    {
        bid = balancing_command["bid"];
        cellBalancingCommand.bid = bid;
    }
    else
    {
        return -1;
    }

    if(balancing_command.containsKey("sbal"))
    {
        status = balancing_command["sbal"];
        cellBalancingCommand.sbal = status;
        JsonArray balancing_command_cball = balancing_command["cball"];
        int arrSize = balancing_command_cball.size();
        for (size_t i = 0; i < arrSize; i++)
        {
            cellBalancingCommand.cball[i] = balancing_command_cball[i];
        }
    }
    else
    {
        return -1;
    }

    return status;
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

int JsonManager::jsonBalancingStatusParser(const char* jsonInput, CellBalancingStatus cellBalancingStatus[])
{
    int bid = 0;
    int startIndex = 0;
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, jsonInput);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return -1;
    }

    if (doc.containsKey("BID"))
    {
        bid = doc["BID"];
        startIndex = bid - 1;
    }
    else
    {
        return -1;
    }

    if (!(doc.containsKey("RBAL1.1") && doc.containsKey("RBAL2.1") && doc.containsKey("RBAL3.1")))
    {
        return -1;
    }

    int rbal[3];
    rbal[0] = doc["RBAL1.1"];
    rbal[1] = doc["RBAL1.2"];
    rbal[2] = doc["RBAL1.3"];
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i] = getBit(i, rbal[0]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+5] = getBit(i, rbal[1]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+10] = getBit(i, rbal[2]);
    }

    rbal[0] = doc["RBAL2.1"];
    rbal[1] = doc["RBAL2.2"];
    rbal[2] = doc["RBAL2.3"];
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+15] = getBit(i, rbal[0]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+20] = getBit(i, rbal[1]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+25] = getBit(i, rbal[2]);
    }

    rbal[0] = doc["RBAL3.1"];
    rbal[1] = doc["RBAL3.2"];
    rbal[2] = doc["RBAL3.3"];
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+30] = getBit(i, rbal[0]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+35] = getBit(i, rbal[1]);
    }
    for (size_t i = 0; i < 5; i++)
    {
        cellBalancingStatus[startIndex].cball[i+40] = getBit(i, rbal[2]);
    }
    return 1;
}

int JsonManager::jsonCMSShutdownParser(const char* jsonInput, CMSShutDown &cmsShutdown)
{
    int command = 0;
    int bid = 0;
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
        cmsShutdown.bid = bid;
    }
    else
    {
        return -1;
    }
    if (!doc.containsKey("shutdown")) 
    {
        return -1;
    }

    cmsShutdown.shutdown = doc["shutdown"]; // 1
    command = cmsShutdown.shutdown;
    return command;
}

int JsonManager::jsonCMSWakeupParser(const char* jsonInput, CMSWakeup &cmsWakeup)
{
    int command = 0;
    int bid = 0;
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
        cmsWakeup.bid = bid;
    }
    else
    {
        return -1;
    }
    if (!doc.containsKey("wakeup")) 
    {
        return -1;
    }
    cmsWakeup.wakeup = doc["wakeup"]; // 1
    command = cmsWakeup.wakeup;
    return command;
}


int JsonManager::getBit(int pos, int data)
{
  if (pos > 7 & pos < 0)
  {
    return -1;
  }
  int temp = data >> pos;
  temp = temp & 0x01;
  return temp;
}