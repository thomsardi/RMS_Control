#include <JsonManager.h>

JsonManager::JsonManager()
{

}

bool JsonManager::isNumber(const String &input)
{
    for (char const &ch : input) {
        if (std::isdigit(ch) == 0) 
            return false;
    }
    return true;
}

int JsonManager::processSingleCmsDataRequest(AsyncWebServerRequest *request)
{
    int bid = 0;
    if(!request->hasParam("bid"))
    {
        return -1;
    }
    String input = request->getParam("bid")->value();
    if(isNumber(input))
    {
        bid = input.toInt();
        return bid;
    }
    else
    {
        return -1;
    }
}

String JsonManager::buildSingleJsonData(const CellData &cellData)
{
    String result;
    StaticJsonDocument<1536> doc; // for 8 object
    doc["msg_count"] = cellData.msgCount;
    doc["frame_name"] = cellData.frameName;
    doc["cms_code"] = cellData.cmsCodeName;
    doc["base_code"] = cellData.baseCodeName;
    doc["mcu_code"] = cellData.mcuCodeName;
    doc["site_location"] = cellData.siteLocation;
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
    doc["wake_status"] = cellData.packStatus.bits.status;
    doc["door_status"] = cellData.packStatus.bits.door;
    serializeJson(doc, result);
    return result;
}

int JsonManager::buildJsonData(AsyncWebServerRequest *request, const PackedData &packedData, String &buffer)
{
    int bid = 0;
    String value[12];
    Vector <String> valueVec;
    valueVec.setStorage(value);
    DynamicJsonDocument doc(12288);
    CellData *pointer;
    doc["rack_sn"] = packedData.rackSn;
    String input = request->getParam("bid")->value();
    if (!request->hasParam("bid"))
    {
        return -1;
    }
    parser(input, ',', valueVec);
    JsonArray cms = doc.createNestedArray("cms_data");
    for (auto &temp : valueVec)
    {
        if(isNumber(temp))
        {
            bid = temp.toInt();
            int index = bid - 1;
            if (index < 0 || index >= packedData.size)
            {
                continue;
            }
            JsonObject cms_0 = cms.createNestedObject();
            pointer = packedData.p + index;
            cms_0["msg_count"] = pointer->msgCount;
            cms_0["frame_name"] = pointer->frameName;
            cms_0["cms_code"] = pointer->cmsCodeName;
            cms_0["base_code"] = pointer->baseCodeName;
            cms_0["mcu_code"] = pointer->mcuCodeName;
            cms_0["site_location"] = pointer->siteLocation;
            cms_0["bid"] = pointer->bid;
            JsonArray vcell = cms_0.createNestedArray("vcell");
            for ( int j = 0; j < 45; j++)
            {
                vcell.add(pointer->vcell[j]);
            }
            JsonArray temp = cms_0.createNestedArray("temp");
            for (int j = 0; j < 9; j++)
            {
                temp.add(pointer->temp[j]);
            }
            JsonArray vpack = cms_0.createNestedArray("pack");
            for (int k = 0; k < 3; k++)
            {
                vpack.add(pointer->pack[k]);
            }
            cms_0["wake_status"] = pointer->packStatus.bits.status;
            cms_0["door_status"] = pointer->packStatus.bits.door;
        }
    }
    serializeJson(doc, buffer);
    return 1;
}

String JsonManager::buildJsonData(AsyncWebServerRequest *request, const PackedData &packedData, const size_t numOfJsonObject) 
{
    String result;
    int bid = 0;
    DynamicJsonDocument doc(12288); //for 8 object
    CellData *pointer;
    doc["rack_sn"] = packedData.rackSn;
    JsonArray cms = doc.createNestedArray("cms_data");
    for (size_t i = 0; i < numOfJsonObject; i++)
    {
        JsonObject cms_0 = cms.createNestedObject();
        pointer = packedData.p + i;
        cms_0["msg_count"] = pointer->msgCount;
        cms_0["frame_name"] = pointer->frameName;
        cms_0["cms_code"] = pointer->cmsCodeName;
        cms_0["base_code"] = pointer->baseCodeName;
        cms_0["mcu_code"] = pointer->mcuCodeName;
        cms_0["site_location"] = pointer->siteLocation;
        cms_0["bid"] = pointer->bid;
        JsonArray vcell = cms_0.createNestedArray("vcell");
        for ( int j = 0; j < 45; j++)
        {
            vcell.add(pointer->vcell[j]);
        }
        JsonArray temp = cms_0.createNestedArray("temp");
        for (int j = 0; j < 9; j++)
        {
            temp.add(pointer->temp[j]);
        }
        JsonArray vpack = cms_0.createNestedArray("pack");
        for (int k = 0; k < 3; k++)
        {
            vpack.add(pointer->pack[k]);
        }
        cms_0["wake_status"] = pointer->packStatus.bits.status;
        cms_0["door_status"] = pointer->packStatus.bits.door;
    }
    serializeJson(doc, result);
    return result;
}

// String JsonManager::buildJsonData(AsyncWebServerRequest *request, const CellData cellData[], const size_t numOfJsonObject) 
// {
//     String result;
//     int bid = 0;
//     DynamicJsonDocument doc(12288); //for 8 object
//     JsonArray cms = doc.createNestedArray("cms_data");
//     for (size_t i = 0; i < numOfJsonObject; i++)
//     {
//         JsonObject cms_0 = cms.createNestedObject();
//         cms_0["msg_count"] = cellData[i].msgCount;
//         cms_0["frame_name"] = cellData[i].frameName;
//         cms_0["cms_code"] = cellData[i].cmsCodeName;
//         cms_0["base_code"] = cellData[i].baseCodeName;
//         cms_0["mcu_code"] = cellData[i].mcuCodeName;
//         cms_0["site_location"] = cellData[i].siteLocation;
//         cms_0["bid"] = cellData[i].bid;
//         JsonArray vcell = cms_0.createNestedArray("vcell");
//         for ( int j = 0; j < 45; j++)
//         {
//             vcell.add(cellData[i].vcell[j]);
//         }
//         JsonArray temp = cms_0.createNestedArray("temp");
//         for (int j = 0; j < 9; j++)
//         {
//             temp.add(cellData[i].temp[j]);
//         }
//         JsonArray vpack = cms_0.createNestedArray("pack");
//         for (int k = 0; k < 3; k++)
//         {
//             vpack.add(cellData[i].pack[k]);
//         }
//         cms_0["wake_status"] = cellData[i].status;
//         cms_0["door_status"] = cellData[i].door;
//     }
//     serializeJson(doc, result);
//     return result;
// }

String JsonManager::buildJsonRMSInfo(const RMSInfo& rmsInfo)
{
    String result;
    DynamicJsonDocument doc(256);
    doc["rms_code"] = rmsInfo.rmsCode;
    doc["rack_sn"] = rmsInfo.rackSn;
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
        cms_info_0["cms_code"] = cmsInfo[i].cmsCodeName;
        cms_info_0["base_code"] = cmsInfo[i].baseCodeName;
        cms_info_0["mcu_code"] = cmsInfo[i].mcuCodeName;
        cms_info_0["site_location"] = cmsInfo[i].siteLocation;
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
    StaticJsonDocument<256> doc;
    doc["vcell_diff"] = alarmParam.vcell_diff;
    doc["vcell_diff_reconnect"] = alarmParam.vcell_diff_reconnect;
    doc["vcell_max"] = alarmParam.vcell_max;
    doc["vcell_min"] = alarmParam.vcell_min;
    doc["vcell_min_reconnect"] = alarmParam.vcell_reconnect;
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

/**
 * @brief   get network information such as ssid and ip
 * @return  json formatted string
*/
String JsonManager::getNetworkInfo(const NetworkSetting &networkSetting)
{
    StaticJsonDocument<256> doc;
    String output;
    doc["ssid"] = networkSetting.ssid;
    doc["ip"] = networkSetting.ip;
    // doc["mode"] = networkSetting.mode;
    serializeJson(doc, output);
    return output;
}

/**
 * @brief   get user network setting
 * @return  json formatted string
*/
String JsonManager::getUserNetworkSetting(const NetworkSetting &networkSetting)
{    
    StaticJsonDocument<256> doc;
    String output;
    doc["ssid"] = networkSetting.ssid;
    doc["pass"] = networkSetting.pass;
    doc["ip"] = networkSetting.ip;
    doc["gateway"] = networkSetting.gateway;
    doc["subnet"] = networkSetting.subnet;
    doc["server"] = networkSetting.server;
    doc["mode"] = networkSetting.mode;
    serializeJson(doc, output);
    return output;
}

/**
 * @brief   get user alarm setting
 * @return  json formatted string
*/
String JsonManager::getUserAlarmSetting(const AlarmParam &alarmParam)
{    
    StaticJsonDocument<256> doc;
    String output;
    doc["vcell_diff"] = alarmParam.vcell_diff;
    doc["vcell_diff_reconnect"] = alarmParam.vcell_diff_reconnect;
    doc["vcell_max"] = alarmParam.vcell_max;
    doc["vcell_min"] = alarmParam.vcell_min;
    doc["vcell_min_reconnect"] = alarmParam.vcell_reconnect;
    doc["temp_max"] = alarmParam.temp_max;
    doc["temp_min"] = alarmParam.temp_min;
    serializeJson(doc, output);
    return output;
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

int JsonManager::jsonLedParser(const char* jsonInput, LedCommand &ledCommand)
{
    int status = -1;
    StaticJsonDocument<768> doc;
    DeserializationError error = deserializeJson(doc, jsonInput);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return status;
    }

    if(!doc.containsKey("bid"))
    {
        return status;
    }

    if(!doc.containsKey("ledset"))
    {
        return status;
    }

    if(!doc.containsKey("num_of_led"))
    {
        return status;
    }

    if(!doc.containsKey("led_rgb"))
    {
        return status;
    }

    ledCommand.bid = doc["bid"];
    ledCommand.ledset = doc["ledset"];
    ledCommand.num_of_led = doc["num_of_led"];

    if(ledCommand.num_of_led <= 0)
    {
        return status;
    }

    JsonArray led_rgb = doc["led_rgb"];
    for (size_t i = 0; i < ledCommand.num_of_led; i++)
    {
        JsonArray led_rgb_value = led_rgb[i];
        ledCommand.red[i] = led_rgb_value[0];
        ledCommand.green[i] = led_rgb_value[1];
        ledCommand.blue[i] = led_rgb_value[2];
    }
    status = 1;
    return status;

}

int JsonManager::jsonAlarmParameterParser(const char* jsonInput, AlarmParam& alarmParam)
{
    StaticJsonDocument<256> doc;

    DeserializationError error = deserializeJson(doc, jsonInput);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return -1;
    }
    if (!doc.containsKey("vcell_diff") || 
        !doc.containsKey("vcell_diff_reconnect") ||
        !doc.containsKey("vcell_max") ||
        !doc.containsKey("vcell_min") ||
        !doc.containsKey("vcell_min_reconnect") ||
        !doc.containsKey("temp_max") ||
        !doc.containsKey("temp_min") ) 
    {
        return -1;
    }
    
    alarmParam.vcell_diff = doc["vcell_diff"];
    alarmParam.vcell_diff_reconnect = doc["vcell_diff_reconnect"];
    alarmParam.vcell_max = doc["vcell_max"];
    alarmParam.vcell_min = doc["vcell_min"];
    alarmParam.vcell_reconnect = doc["vcell_min_reconnect"];
    alarmParam.temp_max = doc["temp_max"].as<signed int>();
    alarmParam.temp_min = doc["temp_min"].as<signed int>();
    return 1;
}

int JsonManager::jsonHardwareAlarmEnableParser(const char* jsonInput, HardwareAlarm& hardwareAlarm)
{
    StaticJsonDocument<128> doc;

    DeserializationError error = deserializeJson(doc, jsonInput);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return -1;
    }
    if (!doc.containsKey("hardware_alarm")) 
    {
        return -1;
    }
    hardwareAlarm.enable = doc["hardware_alarm"];
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

int JsonManager::jsonRmsCodeParser(const char* jsonInput, RmsCodeWrite &rmsCodeWrite)
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

    if (!doc.containsKey("rms_code_write")) 
    {
        return -1;
    }

    rmsCodeWrite.write = doc["rms_code_write"]; // 1
    if (rmsCodeWrite.write)
    {
        rmsCodeWrite.rmsCode = doc["rms_code"].as<String>();
        command = rmsCodeWrite.write;
    }
    Serial.println(rmsCodeWrite.rmsCode);
    return command;
}

int JsonManager::jsonRmsRackSnParser(const char* jsonInput, RmsRackSnWrite &rmsRackSnWrite)
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

    if (!doc.containsKey("rack_sn_write")) 
    {
        return -1;
    }

    rmsRackSnWrite.write = doc["rack_sn_write"]; // 1
    if (rmsRackSnWrite.write)
    {
        rmsRackSnWrite.rackSn = doc["rack_sn"].as<String>();
        command = rmsRackSnWrite.write;
    }
    Serial.println(rmsRackSnWrite.rackSn);
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


int JsonManager::jsonCMSCodeParser(const char* jsonInput, CMSCodeWrite &cmsCodeWrite)
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
        cmsCodeWrite.bid = bid;
    }
    else
    {
        return -1;
    }
    if (!doc.containsKey("cms_write")) 
    {
        return -1;
    }

    cmsCodeWrite.write = doc["cms_write"]; // 1
    if (cmsCodeWrite.write)
    {
        cmsCodeWrite.cmsCode = doc["cms_code"].as<String>();
        command = cmsCodeWrite.write;
    }
    return command;
}

int JsonManager::jsonCMSBaseCodeParser(const char* jsonInput, BaseCodeWrite &baseCodeWrite)
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
        baseCodeWrite.bid = bid;
    }
    else
    {
        return -1;
    }
    if (!doc.containsKey("base_write")) 
    {
        return -1;
    }

    baseCodeWrite.write = doc["base_write"]; // 1
    if (baseCodeWrite.write)
    {
        baseCodeWrite.baseCode = doc["base_code"].as<String>();
        command = baseCodeWrite.write;
    }
    return command;
}

int JsonManager::jsonCMSMcuCodeParser(const char* jsonInput, McuCodeWrite &mcuCodeWrite)
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
        mcuCodeWrite.bid = bid;
    }
    else
    {
        return -1;
    }
    if (!doc.containsKey("mcu_write")) 
    {
        return -1;
    }

    mcuCodeWrite.write = doc["mcu_write"]; // 1
    if (mcuCodeWrite.write)
    {
        mcuCodeWrite.mcuCode = doc["mcu_code"].as<String>();
        command = mcuCodeWrite.write;
    }
    return command;
}

int JsonManager::jsonCMSSiteLocationParser(const char* jsonInput, SiteLocationWrite &siteLocationWrite)
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
        siteLocationWrite.bid = bid;
    }
    else
    {
        return -1;
    }
    if (!doc.containsKey("site_write")) 
    {
        return -1;
    }

    siteLocationWrite.write = doc["site_write"]; // 1
    if (siteLocationWrite.write)
    {
        siteLocationWrite.siteLocation = doc["site_location"].as<String>();
        command = siteLocationWrite.write;
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

int JsonManager::jsonCMSRestartParser(const char* jsonInput, CMSRestartCommand &cmsRestartCommand)
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
        cmsRestartCommand.bid = bid;
    }
    else
    {
        return -1;
    }
    if (!doc.containsKey("restart")) 
    {
        return -1;
    }
    cmsRestartCommand.restart = doc["restart"]; // 1
    command = cmsRestartCommand.restart;
    return command;
}

int JsonManager::jsonCMSRestartPinParser(const char* jsonInput)
{
    int command = 0;
    int bid = 0;
    StaticJsonDocument<32> doc;
    DeserializationError error = deserializeJson(doc, jsonInput);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return -1;
    }

    if (!doc.containsKey("restart")) 
    {
        return -1;
    }
    command = doc["restart"]; // 1
    return command;
}

int JsonManager::jsonRMSRestartParser(const char* jsonInput)
{
    int command = 0;
    int bid = 0;
    StaticJsonDocument<32> doc;
    DeserializationError error = deserializeJson(doc, jsonInput);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return -1;
    }

    if (!doc.containsKey("restart")) 
    {
        return -1;
    }
    command = doc["restart"]; // 1
    return command;
}

int JsonManager::jsonOtaUpdate(const char* jsonInput, OtaParameter &otaParameter)
{
    StaticJsonDocument<192> doc;
    DeserializationError error = deserializeJson(doc, jsonInput);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return -1;
    }

    if (!doc.containsKey("ota_update")) 
    {
        return -1;
    }

    if (!doc.containsKey("server")) 
    {
        return -1;
    }
    if (!doc.containsKey("port")) 
    {
        return -1;
    }
    if (!doc.containsKey("path")) 
    {
        return -1;
    }

    otaParameter.isOtaUpdate = doc["ota_update"];
    otaParameter.server = doc["server"].as<String>();
    otaParameter.port = doc["port"];
    otaParameter.path = doc["path"].as<String>();
    return 1;
}

/**
 * @brief   parse json POST request
 * @return  NetworkSetting struct datatype that contain the setting, set the .flag attribute to 0 when fail, otherwise set to 1
*/
NetworkSetting JsonManager::parseNetworkSetting(JsonVariant &json)
{
    NetworkSetting setting;
    if(!json.containsKey("ssid"))
    {
        return setting;
    }

    if(!json.containsKey("pass"))
    {
        return setting;
    }

    if(!json.containsKey("ip"))
    {
        return setting;
    }
    if(!json.containsKey("gateway"))
    {
        return setting;
    }
    if(!json.containsKey("subnet"))
    {
        return setting;
    }
    if(!json.containsKey("server"))
    {
        return setting;
    }
    if(!json.containsKey("mode"))
    {
        return setting;
    }
    
    auto ssid = json["ssid"].as<String>();
    auto pass = json["pass"].as<String>();
    auto ip = json["ip"].as<String>();
    auto gateway = json["gateway"].as<String>();
    auto subnet = json["subnet"].as<String>();
    auto server = json["server"].as<int8_t>();
    auto mode = json["mode"].as<int8_t>();
    setting.ssid = ssid;
    setting.pass = pass;
    setting.ip = ip;
    setting.gateway = gateway;
    setting.subnet = subnet;
    setting.server = server;
    setting.mode = mode;
    setting.flag = 1;
    return setting;
}

/**
 * @brief   parse json POST request
 * @return  int, -1 when fail, otherwise set to 1
*/
int JsonManager::parseFactoryReset(JsonVariant &json)
{
    int status = -1;
    if (!json.containsKey("factory_reset"))
    {
        return -1;
    }
    
    status = json["factory_reset"];

    return status;
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

void JsonManager::parser(const String &input, char delimiter, Vector<String> &valueVec)
{
    int index = 0;
    int lastIndex = 0;
    int i = 0;
    bool isContinue = true;
    while(isContinue)
    {
        if (i >= valueVec.max_size())
        {
            break;
        }
        lastIndex = input.indexOf(delimiter, index);
        String value = input.substring(index, lastIndex);
        valueVec.push_back(value);
        i++;
        if (lastIndex <= 0)
        {
            isContinue = false;
        }
        index = lastIndex+1;
    }
}