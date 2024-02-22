#ifndef JSONDATA_H
#define JSONDATA_H

#include <Arduino.h>
#include <memory.h>
#include <map>

struct CellData 
{
    uint16_t msgCount = 0;
    String frameName = "FRAME-32-NA";
    String cmsCodeName = "CMS-32-NA";
    String baseCodeName = "BASE-32-NA";
    String mcuCodeName = "MCU-32-NA";
    String siteLocation = "SITE-32-NA";
    String ver = "VER-32-NA";
    String chip = "CHIP-32-NA";
    int bid = 0;
    std::array<int, 45> vcell;
    std::array<int32_t, 9> temp;
    std::array<int32_t, 3> pack;

    union PackStatus {
        struct Bits {
            uint16_t cellDiffAlarm : 1;
            uint16_t cellOvervoltage : 1;
            uint16_t cellUndervoltage : 1;
            uint16_t overtemperature : 1;
            uint16_t undertemperature : 1;
            uint16_t : 3;
            uint16_t status : 1;
            uint16_t door : 1;
            uint16_t : 6;
        } bits;
        uint16_t val;
    } packStatus;
};

struct CMSData
{
    uint16_t msgCount = 0;
    uint8_t errorCount = 0;
    String frameName = "FRAME-32-NA";
    String cmsCodeName = "CMS-32-NA";
    String baseCodeName = "BASE-32-NA";
    String mcuCodeName = "MCU-32-NA";
    String siteLocation = "SITE-32-NA";
    String ver = "VER-32-NA";
    String chip = "CHIP-32-NA";
    int bid = 0;
    std::array<int, 45> vcell;
    std::array<int32_t, 9> temp;
    std::array<int32_t, 3> pack;

    union PackStatus {
        struct Bits {
            uint16_t cellDiffAlarm : 1;
            uint16_t cellOvervoltage : 1;
            uint16_t cellUndervoltage : 1;
            uint16_t overtemperature : 1;
            uint16_t undertemperature : 1;
            uint16_t : 3;
            uint16_t status : 1;
            uint16_t door : 1;
            uint16_t : 6;
        } bits;
        uint16_t val;
    } packStatus;

    CMSData()
    {
        vcell.fill(-1);
        temp.fill(-1);
        pack.fill(-1);
        packStatus.val = 0;
    }

};

struct RMSInfo
{
    String rmsCode = "";
    String rackSn = "";
    String ver = "";
    String ip = "";
    String mac = "";
    String deviceTypeName = "";
};

struct PackedData {
    String *rackSn;
    String *mac;
    std::map<int, CMSData>* cms;
};

struct AlarmParam
{
    uint16_t vcell_diff = 0;
    uint16_t vcell_diff_reconnect = 0;
    uint16_t vcell_overvoltage = 0;
    uint16_t vcell_undervoltage = 0;
    uint16_t vcell_reconnect = 0;
    int32_t temp_max = 0;
    int32_t temp_min = 0;
};

struct CellBalanceState
{
    int bid = 0;
    std::array<int, 45> cball;

    CellBalanceState()
    {
        cball.fill(0);
    }
};

// used to store command from mini-pc
struct CellBalancingCommand
{
    int bid = 0;
    int sbal = 0;
    int cball[45] = {0};
};

struct LedCommand
{
    int bid = 0;
    int ledset = 0;
    int num_of_led = 0;
    int red[8] = {0};
    int green[8] = {0};
    int blue[8] = {0};
};

struct CommandStatus
{
    uint8_t addrCommand = 0;
    uint8_t dataCollectionCommand = 0;
    uint8_t restartCms = 0;
    uint8_t restartRms = 0;
    uint8_t manualOverride = 0;
    uint8_t buzzer = 0;
    uint8_t relay = 0;
    uint8_t factoryReset = 0;
    uint8_t buzzerForce = 0;
    uint8_t relayForce = 0;
};

struct AlarmCommand
{
    int buzzer = 0;
    int powerRelay = 0;
    int battRelay = 0;
};

struct DataCollectionCommand
{
    int exec = 0;
};

struct CMSRestartCommand
{
    int bid = 0;
    int restart = 0;
};

struct RMSRestartCommand
{
    int restart = 0;
};

struct MasterWrite
{
    int write = 0;
    String content = "";
};

struct SlaveWrite
{
    int bid = 0;
    int write = 0;
    String content = "";
};

struct CMSShutDown
{
    int bid = 0;
    int shutdown = 0;
};

struct CMSWakeup
{
    int bid = 0;
    int wakeup = 0;
};

struct AddressingStatus
{
    int numOfDevice = 0;
    int deviceAddressList[8] = {0};
    int status = 0;
};

#endif