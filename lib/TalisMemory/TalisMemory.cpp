#include "TalisMemory.h"

TalisMemory::TalisMemory(/* args */)
{
    esp_log_level_set(_TAG, ESP_LOG_INFO);
}

/**
 * begin
 * 
 * @brief   begin the object by creating a namespace on LittleFS and create a default parameter
 * 
 * @param   storageName namespace for LittleFS
 * @param   params default parameter to save
*/
void TalisMemory::begin(const char* storageName, const TalisDefinition::Params &params)
{
    Preferences _preferences;
    strcpy(_storageName.data(), storageName);
    _preferences.begin(_storageName.data());
    _preferences.end();
    if (!getConfiguredFlag())
    {
        create(params);
        _params = params;
    }
    else
    {
        ESP_LOGI(_TAG, "Load from user setting");
        _params = loadParams();
    }
    _params.print();
}

TalisDefinition::Params TalisMemory::loadParams()
{
    TalisDefinition::Params params;
    params.ssid = getSsid();
    params.pass = getPass();
    params.ip = getIp();
    params.gateway = getGateway();
    params.subnet = getSubnet();
    params.server = getServer();
    params.mode = getMode();
    params.cellDifference = getCellDifference();
    params.cellDifferenceReconnect = getCellDifferenceReconnect();
    params.cellOvervoltage = getCellOvervoltage();
    params.cellUndervoltage = getCellUndervoltage();
    params.cellUndervoltageReconnect = getCellUndervoltageReconnect();
    params.cellOvertemperature = getCellOvertemperature();
    params.cellUndertemperature = getCellUndertemperature();
    params.rmsName = getRmsName();
    params.rackSn = getRackSn();
    return params;
}

/**
 * Get ssid from storage
 * 
 * @return  ssid as string
*/
String TalisMemory::getSsid()
{
    // return _params.ssid;
    Preferences _preferences;
    String value;
    _preferences.begin(_storageName.data(), true);
    value = _preferences.getString(_memoryMap.userMap.network["ssid"].c_str());
    _preferences.end();
    ESP_LOGI(_TAG, "get ssid");
    return value;
}

/**
 * Get pass from storage
 * 
 * @return  pass as string
*/
String TalisMemory::getPass()
{
    // return _params.pass;
    Preferences _preferences;
    String value;
    _preferences.begin(_storageName.data(), true);
    value = _preferences.getString(_memoryMap.userMap.network["pass"].c_str());
    _preferences.end();
    ESP_LOGI(_TAG, "get pass");
    return value;
}

/**
 * Get ip from storage
 * 
 * @return  ip as string
*/
String TalisMemory::getIp()
{
    // return _params.ip;
    Preferences _preferences;
    String value;
    _preferences.begin(_storageName.data(), true);
    value = _preferences.getString(_memoryMap.userMap.network["device_ip"].c_str());
    _preferences.end();
    ESP_LOGI(_TAG, "get ip");
    return value;
}

/**
 * Get gateway from storage
 * 
 * @return  gateway as string
*/
String TalisMemory::getGateway()
{
    // return _params.gateway;
    Preferences _preferences;
    String value;
    _preferences.begin(_storageName.data(), true);
    value = _preferences.getString(_memoryMap.userMap.network["device_gateway"].c_str());
    _preferences.end();
    ESP_LOGI(_TAG, "get gateway");
    return value;
}

/**
 * Get subnet from storage
 * 
 * @return  subnet as string
*/
String TalisMemory::getSubnet()
{
    // return _params.subnet;
    Preferences _preferences;
    String value;
    _preferences.begin(_storageName.data(), true);
    value = _preferences.getString(_memoryMap.userMap.network["device_subnet"].c_str());
    _preferences.end();
    ESP_LOGI(_TAG, "get subnet");
    return value;
}

/**
 * Get server type from storage
 * 
 * @return  server type
*/
uint8_t TalisMemory::getServer(uint8_t valueOnError)
{
    // return _params.server;
    Preferences _preferences;
    uint8_t value;
    _preferences.begin(_storageName.data(), true);
    value = _preferences.getUChar(_memoryMap.userMap.network["server_type"].c_str(), valueOnError);
    _preferences.end();
    return value;
}

/**
 * Get mode from storage
 * 
 * @return  mode
*/
uint8_t TalisMemory::getMode(uint8_t valueOnError)
{
    // return _params.mode;
    Preferences _preferences;
    uint8_t value;
    _preferences.begin(_storageName.data(), true);
    value = _preferences.getUChar(_memoryMap.userMap.network["mode"].c_str(), valueOnError);
    _preferences.end();
    return value;
}

/**
 * Get cell difference parameter from storage
 * 
 * @return  cell difference
*/
uint16_t TalisMemory::getCellDifference(uint16_t valueOnError)
{
    // return _params.cellDifference;
    Preferences _preferences;
    uint16_t value;
    _preferences.begin(_storageName.data(), true);
    value = _preferences.getUShort(_memoryMap.userMap.parameter["cell_difference"].c_str(), valueOnError);
    _preferences.end();
    return value;
}

/**
 * Get cell difference reconnect parameter from storage
 * 
 * @return  cell difference reconnect
*/
uint16_t TalisMemory::getCellDifferenceReconnect(uint16_t valueOnError)
{
    // return _params.cellDifferenceReconnect;
    Preferences _preferences;
    uint16_t value;
    _preferences.begin(_storageName.data(), true);
    value = _preferences.getUShort(_memoryMap.userMap.parameter["cell_difference_reconnect"].c_str(), valueOnError);
    _preferences.end();
    return value;
}

/**
 * Get cell overvoltage parameter from storage
 * 
 * @return  cell overvoltage
*/
uint16_t TalisMemory::getCellOvervoltage(uint16_t valueOnError)
{
    // return _params.cellOvervoltage;
    Preferences _preferences;
    uint16_t value;
    _preferences.begin(_storageName.data(), true);
    value = _preferences.getUShort(_memoryMap.userMap.parameter["cell_overvoltage"].c_str(), valueOnError);
    _preferences.end();
    return value;
}

/**
 * Get cell undervoltage parameter from storage
 * 
 * @return  cell undervoltage
*/
uint16_t TalisMemory::getCellUndervoltage(uint16_t valueOnError)
{
    // return _params.cellUndervoltage;
    Preferences _preferences;
    uint16_t value;
    _preferences.begin(_storageName.data(), true);
    value = _preferences.getUShort(_memoryMap.userMap.parameter["cell_undervoltage"].c_str(), valueOnError);
    _preferences.end();
    return value;
}

/**
 * Get cell undervoltage reconnect parameter from storage
 * 
 * @return  cell undervoltage reconnect
*/
uint16_t TalisMemory::getCellUndervoltageReconnect(uint16_t valueOnError)
{
    // return _params.cellUndervoltageReconnect;
    Preferences _preferences;
    uint16_t value;
    _preferences.begin(_storageName.data(), true);
    value = _preferences.getUShort(_memoryMap.userMap.parameter["cell_undervoltage_reconnect"].c_str(), valueOnError);
    _preferences.end();
    return value;
}

/**
 * Get cell overtemperature parameter from storage
 * 
 * @return  cell overtemperature
*/
int TalisMemory::getCellOvertemperature(int valueOnError)
{
    // return _params.cellOvertemperature;
    Preferences _preferences;
    int value;
    _preferences.begin(_storageName.data(), true);
    value = _preferences.getInt(_memoryMap.userMap.parameter["cell_overtemperature"].c_str(), valueOnError);
    _preferences.end();
    return value;
}

/**
 * Get cell undertemperature parameter from storage
 * 
 * @return  cell undertemperature
*/
int TalisMemory::getCellUndertemperature(int valueOnError)
{
    // return _params.cellUndertemperature;
    Preferences _preferences;
    int value;
    _preferences.begin(_storageName.data(), true);
    value = _preferences.getInt(_memoryMap.userMap.parameter["cell_undertemperature"].c_str(), valueOnError);
    _preferences.end();
    return value;
}

/**
 * Get rms name from storage
 * 
 * @return  rms name as string
*/
String TalisMemory::getRmsName()
{
    // return _params.rmsName;
    Preferences _preferences;
    String value;
    _preferences.begin(_storageName.data(), true);
    value = _preferences.getString(_memoryMap.userMap.parameter["rms_name"].c_str());
    _preferences.end();
    return value;
}

/**
 * Get rack sn from storage
 * 
 * @return  rack sn as string
*/
String TalisMemory::getRackSn()
{
    // return _params.rackSn;
    Preferences _preferences;
    String value;
    _preferences.begin(_storageName.data(), true);
    value = _preferences.getString(_memoryMap.userMap.parameter["rack_sn"].c_str());
    _preferences.end();
    return value;
}

/**
 * Get configured flag from storage
 * 
 * @brief   when the namespace / factory reset called, the configured flag will be reset to 0. this will force re-create or 
 *          overwrite the current setting with default value during .begin() method
 * 
 * @return  boolean value
*/
bool TalisMemory::getConfiguredFlag()
{
    Preferences _preferences;
    bool value;
    _preferences.begin(_storageName.data());

    if (!_preferences.isKey(_memoryMap.defaultMap.flag["configured_flag"].c_str())) // check if the configured flag is not defined yet
    {
        _preferences.putBool(_memoryMap.defaultMap.flag["configured_flag"].c_str(), false);   // fill the configured flag with 0
    }
    
    value = _preferences.getBool(_memoryMap.defaultMap.flag["configured_flag"].c_str());
    _preferences.end();
    return value;
}

/**
 * set ssid into storage
 * 
 * @param   value   ssid name
 * @return  true if success, otherwise false
*/
bool TalisMemory::setSsid(const char* value)
{
    Preferences _preferences;
    bool status;
    _preferences.begin(_storageName.data());
    if (_preferences.putString(_memoryMap.userMap.network["ssid"].c_str(), value))
    {
        status = true;
        _params.ssid = String(value);
    }
    _preferences.end();
    return status;
}

/**
 * set pass into storage
 * 
 * @param   value   pass name
 * @return  true if success, otherwise false
*/
bool TalisMemory::setPass(const char* value)
{
    Preferences _preferences;
    bool status;
    _preferences.begin(_storageName.data());
    if (_preferences.putString(_memoryMap.userMap.network["pass"].c_str(), value))
    {
        status = true;
        _params.pass = String(value);
    }
    _preferences.end();
    return status;
}

/**
 * set ip into storage
 * 
 * @param   value   ip as string
 * @return  true if success, otherwise false
*/
bool TalisMemory::setIp(const char* value)
{
    Preferences _preferences;
    bool status;
    _preferences.begin(_storageName.data());
    if (_preferences.putString(_memoryMap.userMap.network["device_ip"].c_str(), value))
    {
        status = true;
        _params.ip = String(value);
    }
    _preferences.end();
    return status;
}

/**
 * set gateway into storage
 * 
 * @param   value   gateway as string
 * @return  true if success, otherwise false
*/
bool TalisMemory::setGateway(const char* value)
{
    Preferences _preferences;
    bool status;
    _preferences.begin(_storageName.data());
    if (_preferences.putString(_memoryMap.userMap.network["device_gateway"].c_str(), value))
    {
        status = true;
        _params.gateway = String(value);
    }
    _preferences.end();
    return status;
}

/**
 * set subnet into storage
 * 
 * @param   value   subnet as string
 * @return  true if success, otherwise false
*/
bool TalisMemory::setSubnet(const char* value)
{
    Preferences _preferences;
    bool status;
    _preferences.begin(_storageName.data());
    if (_preferences.putString(_memoryMap.userMap.network["device_subnet"].c_str(), value))
    {
        status = true;
        _params.subnet = String(value);
    }
    _preferences.end();
    return status;
}

/**
 * set server type into storage
 * 
 * @param   value   server type (0 - 2)
 * @return  true if success, otherwise false
*/
bool TalisMemory::setServer(uint8_t value)
{
    Preferences _preferences;
    bool status;
    _preferences.begin(_storageName.data());
    if (_preferences.putUChar(_memoryMap.userMap.network["server_type"].c_str(), value))
    {
        status = true;
        _params.server = value;
    }
    _preferences.end();
    return status;
}

/**
 * set mode into storage
 * 
 * @param   value   mode (0 - 1)
 * @return  true if success, otherwise false
*/
bool TalisMemory::setMode(uint8_t value)
{
    Preferences _preferences;
    bool status;
    _preferences.begin(_storageName.data());
    if (_preferences.putUChar(_memoryMap.userMap.network["mode"].c_str(), value))
    {
        status = true;
        _params.mode = value;
    }
    _preferences.end();
    return status;
}

/**
 * set cell difference into storage
 * 
 * @param   value   cell difference
 * @return  true if success, otherwise false
*/
bool TalisMemory::setCellDifference(uint16_t value)
{
    Preferences _preferences;
    bool status;
    _preferences.begin(_storageName.data());
    if (_preferences.putUShort(_memoryMap.userMap.parameter["cell_difference"].c_str(), value))
    {
        status = true;
        _params.cellDifference = value;
    }
    _preferences.end();
    return status;
}

/**
 * set cell difference reconnect into storage
 * 
 * @param   value   cell difference reconnect
 * @return  true if success, otherwise false
*/
bool TalisMemory::setCellDifferenceReconnect(uint16_t value)
{
    Preferences _preferences;
    bool status;
    _preferences.begin(_storageName.data());
    if (_preferences.putUShort(_memoryMap.userMap.parameter["cell_difference_reconnect"].c_str(), value))
    {
        status = true;
        _params.cellDifferenceReconnect = value;
    }
    _preferences.end();
    return status;
}

/**
 * set cell overvoltage into storage
 * 
 * @param   value   cell overvoltage
 * @return  true if success, otherwise false
*/
bool TalisMemory::setCellOvervoltage(uint16_t value)
{
    Preferences _preferences;
    bool status;
    _preferences.begin(_storageName.data());
    if (_preferences.putUShort(_memoryMap.userMap.parameter["cell_overvoltage"].c_str(), value))
    {
        status = true;
        _params.cellOvervoltage = value;
    }
    _preferences.end();
    return status;
}

/**
 * set cell undervoltage into storage
 * 
 * @param   value   cell undervoltage
 * @return  true if success, otherwise false
*/
bool TalisMemory::setCellUndervoltage(uint16_t value)
{
    Preferences _preferences;
    bool status;
    _preferences.begin(_storageName.data());
    if (_preferences.putUShort(_memoryMap.userMap.parameter["cell_undervoltage"].c_str(), value))
    {
        status = true;
        _params.cellUndervoltage = value;
    }
    _preferences.end();
    return status;
}

/**
 * set cell undervoltage reconnect into storage
 * 
 * @param   value   cell undervoltage reconnect
 * @return  true if success, otherwise false
*/
bool TalisMemory::setCellUndervoltageReconnect(uint16_t value)
{
    Preferences _preferences;
    bool status;
    _preferences.begin(_storageName.data());
    if (_preferences.putUShort(_memoryMap.userMap.parameter["cell_undervoltage_reconnect"].c_str(), value))
    {
        status = true;
        _params.cellUndervoltageReconnect = value;
    }
    _preferences.end();
    return status;
}

/**
 * set cell overtemperature into storage
 * 
 * @param   value   cell overtemperature
 * @return  true if success, otherwise false
*/
bool TalisMemory::setCellOvertemperature(int value)
{
    Preferences _preferences;
    bool status;
    _preferences.begin(_storageName.data());
    if (_preferences.putInt(_memoryMap.userMap.parameter["cell_overtemperature"].c_str(), value))
    {
        status = true;
        _params.cellOvertemperature = value;
    }
    _preferences.end();
    return status;
}

/**
 * set cell undertemperature into storage
 * 
 * @param   value   cell undertemperature
 * @return  true if success, otherwise false
*/
bool TalisMemory::setCellUndertemperature(int value)
{
    Preferences _preferences;
    bool status;
    _preferences.begin(_storageName.data());
    if (_preferences.putInt(_memoryMap.userMap.parameter["cell_undertemperature"].c_str(), value))
    {
        status = true;
        _params.cellUndertemperature = value;
    }
    _preferences.end();
    return status;
}

/**
 * set rms name into storage
 * 
 * @param   value   rms name
 * @return  true if success, otherwise false
*/
bool TalisMemory::setRmsName(const char* value)
{
    Preferences _preferences;
    bool status;
    _preferences.begin(_storageName.data());
    if (_preferences.putString(_memoryMap.userMap.parameter["rms_name"].c_str(), value))
    {
        status = true;
        _params.rmsName = String(value);
    }
    _preferences.end();
    return status;
}

/**
 * set rms name into storage
 * 
 * @param   value   rms name
 * @return  true if success, otherwise false
*/
bool TalisMemory::setRackSn(const char* value)
{
    Preferences _preferences;
    bool status;
    _preferences.begin(_storageName.data());
    if (_preferences.putString(_memoryMap.userMap.parameter["rack_sn"].c_str(), value))
    {
        status = true;
        _params.rackSn = String(value);
    }
    _preferences.end();
    return status;
}

/**
 * Set configured flag to storage
 * 
 * @param   value   the value to be written into
 * 
 * @return  true if success, otherwise false
*/
bool TalisMemory::setConfiguredFlag(bool value)
{
    Preferences _preferences;
    bool status = false;
    _preferences.begin(_storageName.data());
    if (_preferences.putBool(_memoryMap.defaultMap.flag["configured_flag"].c_str(), value))
    {
        status = true;
    }
    _preferences.end();
    return status;
}

/**
 * Create default parameter
*/
void TalisMemory::create(const TalisDefinition::Params &params)
{
    enum server_type : uint8_t {
        STATIC = 1,
        DHCP = 2
    };
    enum mode_type : uint8_t {
        AP = 1,
        STATION = 2,
        AP_STATION = 3
    };
    
    ESP_LOGI(_TAG, "Creating default parameters");
    // return;
    Preferences _preferences;
    _preferences.begin(_storageName.data());

    /**
     * Create default setting
    */

    // Set default network
    _preferences.putString(_memoryMap.defaultMap.network["ssid"].c_str(), params.ssid);
    _preferences.putString(_memoryMap.defaultMap.network["pass"].c_str(), params.pass);
    _preferences.putString(_memoryMap.defaultMap.network["device_ip"].c_str(), params.ip);
    _preferences.putString(_memoryMap.defaultMap.network["device_gateway"].c_str(), params.gateway);
    _preferences.putString(_memoryMap.defaultMap.network["device_subnet"].c_str(), params.subnet);
    _preferences.putUChar(_memoryMap.defaultMap.network["server_type"].c_str(), params.server);
    _preferences.putUChar(_memoryMap.defaultMap.network["mode"].c_str(), params.mode);
    
    // Set default parameter
    _preferences.putUShort(_memoryMap.defaultMap.parameter["cell_difference"].c_str(), params.cellDifference);
    _preferences.putUShort(_memoryMap.defaultMap.parameter["cell_difference_reconnect"].c_str(), params.cellDifferenceReconnect);
    _preferences.putUShort(_memoryMap.defaultMap.parameter["cell_overvoltage"].c_str(), params.cellOvervoltage);
    _preferences.putUShort(_memoryMap.defaultMap.parameter["cell_undervoltage"].c_str(), params.cellUndervoltage);
    _preferences.putUShort(_memoryMap.defaultMap.parameter["cell_undervoltage_reconnect"].c_str(), params.cellUndervoltageReconnect);
    _preferences.putInt(_memoryMap.defaultMap.parameter["cell_overtemperature"].c_str(), params.cellOvertemperature);
    _preferences.putInt(_memoryMap.defaultMap.parameter["cell_undertemperature"].c_str(), params.cellUndertemperature);
    _preferences.putString(_memoryMap.defaultMap.parameter["rms_name"].c_str(), params.rmsName);
    _preferences.putString(_memoryMap.defaultMap.parameter["rack_sn"].c_str(), params.rackSn);
    
    /**
     * Copy default setting into user setting
    */

    // Set user network
    _preferences.putString(_memoryMap.userMap.network["ssid"].c_str(), _preferences.getString(_memoryMap.defaultMap.network["ssid"].c_str()));
    _preferences.putString(_memoryMap.userMap.network["pass"].c_str(), _preferences.getString(_memoryMap.defaultMap.network ["pass"].c_str()));
    _preferences.putString(_memoryMap.userMap.network["device_ip"].c_str(), _preferences.getString(_memoryMap.defaultMap.network["device_ip"].c_str()));
    _preferences.putString(_memoryMap.userMap.network["device_gateway"].c_str(), _preferences.getString(_memoryMap.defaultMap.network["device_gateway"].c_str()));
    _preferences.putString(_memoryMap.userMap.network["device_subnet"].c_str(), _preferences.getString(_memoryMap.defaultMap.network["device_subnet"].c_str()));
    _preferences.putUChar(_memoryMap.userMap.network["server_type"].c_str(), _preferences.getUChar(_memoryMap.defaultMap.network["server_type"].c_str()));
    _preferences.putUChar(_memoryMap.userMap.network["mode"].c_str(), _preferences.getUChar(_memoryMap.defaultMap.network["mode"].c_str()));

    // Set user parameter
    _preferences.putUShort(_memoryMap.userMap.parameter["cell_difference"].c_str(), _preferences.getUShort(_memoryMap.defaultMap.parameter["cell_difference"].c_str()));
    _preferences.putUShort(_memoryMap.userMap.parameter["cell_difference_reconnect"].c_str(), _preferences.getUShort(_memoryMap.defaultMap.parameter["cell_difference_reconnect"].c_str()));
    _preferences.putUShort(_memoryMap.userMap.parameter["cell_overvoltage"].c_str(), _preferences.getUShort(_memoryMap.defaultMap.parameter["cell_overvoltage"].c_str()));
    _preferences.putUShort(_memoryMap.userMap.parameter["cell_undervoltage"].c_str(), _preferences.getUShort(_memoryMap.defaultMap.parameter["cell_undervoltage"].c_str()));
    _preferences.putUShort(_memoryMap.userMap.parameter["cell_undervoltage_reconnect"].c_str(), _preferences.getUShort(_memoryMap.defaultMap.parameter["cell_undervoltage_reconnect"].c_str()));
    _preferences.putInt(_memoryMap.userMap.parameter["cell_overtemperature"].c_str(), _preferences.getInt(_memoryMap.defaultMap.parameter["cell_overtemperature"].c_str()));
    _preferences.putInt(_memoryMap.userMap.parameter["cell_undertemperature"].c_str(), _preferences.getInt(_memoryMap.defaultMap.parameter["cell_undertemperature"].c_str()));
    _preferences.putString(_memoryMap.userMap.parameter["rms_name"].c_str(), _preferences.getString(_memoryMap.defaultMap.parameter["rms_name"].c_str()));
    _preferences.putString(_memoryMap.userMap.parameter["rack_sn"].c_str(), _preferences.getString(_memoryMap.defaultMap.parameter["rack_sn"].c_str()));

    _preferences.putBool(_memoryMap.defaultMap.flag["configured_flag"].c_str(), 1);

    _preferences.end();
}

/**
 * Reset configured flag of storage
 * 
 * @return  true if success, otherwise false
*/
bool TalisMemory::reset()
{
    return setConfiguredFlag(false);
}

/**
 * print the content of LittleFS
 * 
 * @brief   print the content of LittleFS according to the namespace set during begin
*/
void TalisMemory::print()
{
    
    Preferences _preferences;
    _preferences.begin(_storageName.data(), true);

    ESP_LOGI(_TAG, "====Default====\n");
    ESP_LOGI(_TAG, "Key %s : %s\n", _memoryMap.defaultMap.network["ssid"].c_str(), _preferences.getString(_memoryMap.defaultMap.network["ssid"].c_str()).c_str());
    ESP_LOGI(_TAG, "Key %s : %s\n", _memoryMap.defaultMap.network["pass"].c_str(), _preferences.getString(_memoryMap.defaultMap.network["pass"].c_str()).c_str());
    ESP_LOGI(_TAG, "Key %s : %s\n", _memoryMap.defaultMap.network["device_ip"].c_str(), _preferences.getString(_memoryMap.defaultMap.network["device_ip"].c_str()).c_str());
    ESP_LOGI(_TAG, "Key %s : %s\n", _memoryMap.defaultMap.network["device_gateway"].c_str(), _preferences.getString(_memoryMap.defaultMap.network["device_gateway"].c_str()).c_str());
    ESP_LOGI(_TAG, "Key %s : %s\n", _memoryMap.defaultMap.network["device_subnet"].c_str(), _preferences.getString(_memoryMap.defaultMap.network["device_subnet"].c_str()).c_str());
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.defaultMap.network["server_type"].c_str(), _preferences.getUChar(_memoryMap.defaultMap.network["server_type"].c_str()));
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.defaultMap.network["mode"].c_str(), _preferences.getUChar(_memoryMap.defaultMap.network["mode"].c_str()));
    
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.defaultMap.parameter["cell_difference"].c_str(), _preferences.getUShort(_memoryMap.defaultMap.parameter["cell_difference"].c_str()));
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.defaultMap.parameter["cell_difference_reconnect"].c_str(), _preferences.getUShort(_memoryMap.defaultMap.parameter["cell_difference_reconnect"].c_str()));
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.defaultMap.parameter["cell_overvoltage"].c_str(), _preferences.getUShort(_memoryMap.defaultMap.parameter["cell_overvoltage"].c_str()));
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.defaultMap.parameter["cell_undervoltage"].c_str(), _preferences.getUShort(_memoryMap.defaultMap.parameter["cell_undervoltage"].c_str()));
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.defaultMap.parameter["cell_undervoltage_reconnect"].c_str(), _preferences.getUShort(_memoryMap.defaultMap.parameter["cell_undervoltage_reconnect"].c_str()));
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.defaultMap.parameter["cell_overtemperature"].c_str(), _preferences.getInt(_memoryMap.defaultMap.parameter["cell_overtemperature"].c_str()));
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.defaultMap.parameter["cell_undertemperature"].c_str(), _preferences.getInt(_memoryMap.defaultMap.parameter["cell_undertemperature"].c_str()));
    ESP_LOGI(_TAG, "Key %s : %s\n", _memoryMap.defaultMap.parameter["rms_name"].c_str(), _preferences.getString(_memoryMap.defaultMap.parameter["rms_name"].c_str()).c_str());
    ESP_LOGI(_TAG, "Key %s : %s\n", _memoryMap.defaultMap.parameter["rack_sn"].c_str(), _preferences.getString(_memoryMap.defaultMap.parameter["rack_sn"].c_str()).c_str());

    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.defaultMap.flag["configured_flag"].c_str(), _preferences.getChar(_memoryMap.defaultMap.flag["configured_flag"].c_str()));
    
    ESP_LOGI(_TAG, "====User====\n");    
    ESP_LOGI(_TAG, "Key %s : %s\n", _memoryMap.userMap.network["ssid"].c_str(), _preferences.getString(_memoryMap.userMap.network["ssid"].c_str()).c_str());
    ESP_LOGI(_TAG, "Key %s : %s\n", _memoryMap.userMap.network["pass"].c_str(), _preferences.getString(_memoryMap.userMap.network["pass"].c_str()).c_str());
    ESP_LOGI(_TAG, "Key %s : %s\n", _memoryMap.userMap.network["device_ip"].c_str(), _preferences.getString(_memoryMap.userMap.network["device_ip"].c_str()).c_str());
    ESP_LOGI(_TAG, "Key %s : %s\n", _memoryMap.userMap.network["device_gateway"].c_str(), _preferences.getString(_memoryMap.userMap.network["device_gateway"].c_str()).c_str());
    ESP_LOGI(_TAG, "Key %s : %s\n", _memoryMap.userMap.network["device_subnet"].c_str(), _preferences.getString(_memoryMap.userMap.network["device_subnet"].c_str()).c_str());
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.userMap.network["server_type"].c_str(), _preferences.getUChar(_memoryMap.userMap.network["server_type"].c_str()));
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.userMap.network["mode"].c_str(), _preferences.getUChar(_memoryMap.userMap.network["mode"].c_str()));
    
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.userMap.parameter["cell_difference"].c_str(), _preferences.getUShort(_memoryMap.userMap.parameter["cell_difference"].c_str()));
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.userMap.parameter["cell_difference_reconnect"].c_str(), _preferences.getUShort(_memoryMap.userMap.parameter["cell_difference_reconnect"].c_str()));
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.userMap.parameter["cell_overvoltage"].c_str(), _preferences.getUShort(_memoryMap.userMap.parameter["cell_overvoltage"].c_str()));
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.userMap.parameter["cell_undervoltage"].c_str(), _preferences.getUShort(_memoryMap.userMap.parameter["cell_undervoltage"].c_str()));
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.userMap.parameter["cell_undervoltage_reconnect"].c_str(), _preferences.getUShort(_memoryMap.userMap.parameter["cell_undervoltage_reconnect"].c_str()));
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.userMap.parameter["cell_overtemperature"].c_str(), _preferences.getInt(_memoryMap.userMap.parameter["cell_overtemperature"].c_str()));
    ESP_LOGI(_TAG, "Key %s : %d\n", _memoryMap.userMap.parameter["cell_undertemperature"].c_str(), _preferences.getInt(_memoryMap.userMap.parameter["cell_undertemperature"].c_str()));
    ESP_LOGI(_TAG, "Key %s : %s\n", _memoryMap.userMap.parameter["rms_name"].c_str(), _preferences.getString(_memoryMap.userMap.parameter["rms_name"].c_str()).c_str());
    ESP_LOGI(_TAG, "Key %s : %s\n", _memoryMap.userMap.parameter["rack_sn"].c_str(), _preferences.getString(_memoryMap.userMap.parameter["rack_sn"].c_str()).c_str());

    _preferences.end();

}

TalisMemory::~TalisMemory()
{

}