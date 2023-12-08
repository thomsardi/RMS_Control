#ifndef TALIS_MEMORY_H
#define TALIS_MEMORY_H

#include <map>
#include "esp_log.h"
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

namespace TalisDefinition {
    struct Params
    {
        String ssid = "esp32-ssid";
        String pass = "esp32-pass";
        String ip = "192.168.4.1";
        String gateway = "192.168.4.1";
        String subnet = "255.255.255.0";
        uint8_t server = 1;
        uint8_t mode = 1;
        uint16_t cellDifference = 300;
        uint16_t cellDifferenceReconnect = 250;
        uint16_t cellOvervoltage = 3700;
        uint16_t cellUndervoltage = 2800;
        uint16_t cellUndervoltageReconnect = 3000;
        int cellOvertemperature = 80000;
        int cellUndertemperature = 10000;
        String rmsName = "RMS-32-NA";
        String rackSn = "RACK-32-NA";

        Params()
        {
            esp_log_level_set("Params", ESP_LOG_INFO);
        }
        
        void print()
        {
            ESP_LOGI("Params", "SSID : %s\n", ssid.c_str());
            ESP_LOGI("Params", "Pass : %s\n", pass.c_str());
            ESP_LOGI("Params", "IP : %s\n", ip.c_str());
            ESP_LOGI("Params", "Gateway : %s\n", gateway.c_str());
            ESP_LOGI("Params", "Subnet : %s\n", subnet.c_str());
            ESP_LOGI("Params", "Server : %d\n", server);
            ESP_LOGI("Params", "Mode : %d\n", mode);
            ESP_LOGI("Params", "Cell Difference : %d\n", cellDifference);
            ESP_LOGI("Params", "Cell Difference Reconnect : %d\n", cellDifferenceReconnect);
            ESP_LOGI("Params", "Cell Overvoltage : %d\n", cellOvervoltage);
            ESP_LOGI("Params", "Cell Undervoltage : %d\n", cellUndervoltage);
            ESP_LOGI("Params", "Cell Undervoltage Reconnect : %d\n", cellUndervoltageReconnect);
            ESP_LOGI("Params", "Cell Overtemperature : %d\n", cellOvertemperature);
            ESP_LOGI("Params", "Cell Undertemperature : %d\n", cellUndertemperature);
            ESP_LOGI("Params", "RMS Name : %s\n", rmsName.c_str());
            ESP_LOGI("Params", "Rack SN : %s\n", rackSn.c_str());
        }

        Params& operator=(const Params &params)
        {
            this->ssid = params.ssid;
            this->pass = params.pass;
            this->ip = params.ip;
            this->gateway = params.gateway;
            this->subnet = params.subnet;
            this->server = params.server;
            this->mode = params.mode;
            this->cellDifference = params.cellDifference;
            this->cellDifferenceReconnect = params.cellDifferenceReconnect;
            this->cellOvervoltage = params.cellOvervoltage;
            this->cellUndervoltage = params.cellUndervoltage;
            this->cellUndervoltageReconnect = params.cellUndervoltageReconnect;
            this->cellOvertemperature = params.cellOvertemperature;
            this->cellUndertemperature = params.cellUndertemperature;
            this->rmsName = params.rmsName;
            this->rackSn = params.rackSn;
            return *this;
        }

    };
    
    struct MemoryMap 
    {
        struct DefaultMemoryMap
        {
            const char* _TAG = "Default Memory Map";
            std::map<std::string, std::string> network = {
                {"ssid", "d_ssid"},
                {"pass", "d_pass"},
                {"device_ip", "d_ip"},
                {"device_gateway", "d_gateway"},
                {"device_subnet", "d_subnet"},
                {"server_type", "d_server"},
                {"mode", "d_mode"}
            };

            std::map<std::string, std::string> parameter = {
                {"cell_difference", "d_cdiff"},
                {"cell_difference_reconnect", "d_cdiff_r"},
                {"cell_overvoltage", "d_coverv"},
                {"cell_undervoltage", "d_cunderv"},
                {"cell_undervoltage_reconnect", "d_cunderv_r"},
                {"cell_overtemperature", "d_covert"},
                {"cell_undertemperature", "d_cundert"},
                {"rms_name", "d_rms_n"},
                {"rack_sn", "d_rack_sn"},
            };

            std::map<std::string, std::string> flag = {
                {"configured_flag", "s_flag"}
            };

            DefaultMemoryMap()
            {
                esp_log_level_set(_TAG, ESP_LOG_INFO);
            }

            void print() 
            {
                ESP_LOGI(_TAG, "Default Network Key List :\n");
                std::map<std::string, std::string>::iterator it;
                for (it = network.begin(); it != network.end(); it++)
                {
                    ESP_LOGI(_TAG, "%s : %s\n", (*it).first, (*it).second);
                }

                ESP_LOGI(_TAG, "Default Parameter Key List");
                for (it = parameter.begin(); it != parameter.end(); it++)
                {
                    ESP_LOGI(_TAG, "%s : %s\n", (*it).first, (*it).second);
                }

                ESP_LOGI(_TAG, "Default Flag Key List");
                for (it = flag.begin(); it != flag.end(); it++)
                {
                    ESP_LOGI(_TAG, "%s : %s\n", (*it).first, (*it).second);
                }
            }
        } defaultMap;

        struct UserMemoryMap
        {
            const char* _TAG = "User Memory Map";
            std::map<std::string, std::string> network = {
                {"ssid", "u_ssid"},
                {"pass", "u_pass"},
                {"device_ip", "u_ip"},
                {"device_gateway", "u_gateway"},
                {"device_subnet", "u_subnet"},
                {"server_type", "u_server"},
                {"mode", "u_mode"},
            };

            std::map<std::string, std::string> parameter = {
                {"cell_difference", "u_cdiff"},
                {"cell_difference_reconnect", "u_cdiff_r"},
                {"cell_overvoltage", "u_coverv"},
                {"cell_undervoltage", "u_cunderv"},
                {"cell_undervoltage_reconnect", "u_cunderv_r"},
                {"cell_overtemperature", "u_covert"},
                {"cell_undertemperature", "u_cundert"},
                {"rms_name", "u_rms_n"},
                {"rack_sn", "u_rack_sn"},
            };

            void print() 
            {
                ESP_LOGI(_TAG, "User Network Key List :\n");
                std::map<std::string, std::string>::iterator it;
                for (it = network.begin(); it != network.end(); it++)
                {
                    ESP_LOGI(_TAG, "%s : %s\n", (*it).first, (*it).second);
                }

                ESP_LOGI(_TAG, "User Parameter Key List");
                for (it = parameter.begin(); it != parameter.end(); it++)
                {
                    ESP_LOGI(_TAG, "%s : %s\n", (*it).first, (*it).second);
                }
            }

        } userMap;
    };
}

class TalisMemory
{
private:
    /* data */
    const char* _TAG = "Talis Memory";
    bool getConfiguredFlag();
    bool setConfiguredFlag(bool value);
    void create(const TalisDefinition::Params &params);
    TalisDefinition::Params loadParams();
    std::array<char, 32> _storageName;
    TalisDefinition::Params _params;
    TalisDefinition::MemoryMap _memoryMap;
public:
    TalisMemory(/* args */);
    void begin(const char* storageName, const TalisDefinition::Params &params);
    void print();
    String getSsid();
    String getPass();
    String getIp();
    String getGateway();
    String getSubnet();
    uint8_t getServer(uint8_t valueOnError = 0);
    uint8_t getMode(uint8_t valueOnError = 0);
    uint16_t getCellDifference(uint16_t valueOnError = 0);
    uint16_t getCellDifferenceReconnect(uint16_t valueOnError = 0);
    uint16_t getCellOvervoltage(uint16_t valueOnError = 0);
    uint16_t getCellUndervoltage(uint16_t valueOnError = 0);
    uint16_t getCellUndervoltageReconnect(uint16_t valueOnError = 0);
    int getCellOvertemperature(int valueOnError = 0);
    int getCellUndertemperature(int valueOnError = 0);
    String getRmsName();
    String getRackSn();
    
    bool setSsid(const char* value);
    bool setPass(const char* value);
    bool setIp(const char* value);
    bool setGateway(const char* value);
    bool setSubnet(const char* value);
    bool setServer(uint8_t value);
    bool setMode(uint8_t value);
    bool setCellDifference(uint16_t value);
    bool setCellDifferenceReconnect(uint16_t value);
    bool setCellOvervoltage(uint16_t value);
    bool setCellUndervoltage(uint16_t value);
    bool setCellUndervoltageReconnect(uint16_t value);
    bool setCellOvertemperature(int value);
    bool setCellUndertemperature(int value);
    bool setRmsName(const char* value);
    bool setRackSn(const char* value);

    bool reset();
    ~TalisMemory();
};





#endif