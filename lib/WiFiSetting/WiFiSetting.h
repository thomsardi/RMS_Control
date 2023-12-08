#ifndef WIFI_SETTING_H
#define WIFI_SETTING_H

#include <WiFi.h>
#include <memory>

enum server_type : uint8_t {
    STATIC = 1,
    DHCP = 2
};
enum mode_type : uint8_t {
    AP = 1,
    STATION = 2,
    AP_STATION = 3
};

struct ConfigParams {
    std::array<char, 32> ssid;
    std::array<char, 32> pass;
    std::array<char, 32> ip;
    std::array<char, 32> gateway;
    std::array<char, 32> subnet;
    uint8_t server;
};


struct WifiParams
{
    uint8_t mode;
    ConfigParams params;
    ConfigParams softApParams;
};

class WiFiSetting
{
private:
    /* data */
    mode_type _modeType = AP;
    server_type _stationServerType = DHCP;
    server_type _apServerType = DHCP;
    String _apGateway;
    const char* _TAG = "WiFi Setting";
public:
    WiFiSetting();
    void begin(WifiParams &wifiParams);
    void start();
    bool setServer(server_type serverType, IPAddress ip = (uint32_t)0, IPAddress gateway = (uint32_t)0, IPAddress subnet = (uint32_t)0);
    void setMode(mode_type modeType);
    bool setApServer(server_type serverType, IPAddress ip = (uint32_t)0, IPAddress gateway = (uint32_t)0, IPAddress subnet = (uint32_t)0);
    void setApCredential(String ssid, String pass);
    void setCredential(String ssid, String pass);
    mode_type getMode();
    
    server_type getStationServer();
    String getIp();
    String getGateway();
    String getSubnet();
    String getSsid();

    server_type getApServer();
    String getApIp();
    String getApGateway();
    String getApSubnet();
    String getApSsid();
    ~WiFiSetting();
};




#endif