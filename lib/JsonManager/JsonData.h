#ifndef JSONDATA_H
#define JSONDATA_H

#include <Arduino.h>

struct CellData
{
    uint16_t msgCount = 0;
    String rackSn = "";
    String frameName = "";
    String cmsCodeName = "";
    String baseCodeName = "";
    String mcuCodeName = "";
    String siteLocation = "";
    int bid = 0;
    int vcell[45] = {0};
    int32_t temp[9] = {0};
    int32_t pack[3] = {0};

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

    // int status = 0;
    // int door = 0;
};

struct Data {
    String rackSn = "";
    CellData *p;
    int size;
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

struct CMSInfo
{
    String frameName = "";
    int bid = 0;
    String cmsCodeName = "";
    String baseCodeName = "";
    String mcuCodeName = "";
    String siteLocation = "";
    String ver = "";
    String chip = "";
};


struct AlarmParam
{
    uint16_t vcell_diff = 0;
    uint16_t vcell_diff_reconnect = 0;
    uint16_t vcell_max = 0;
    uint16_t vcell_min = 0;
    uint16_t vcell_reconnect = 0;
    int32_t temp_max = 0;
    int32_t temp_min = 0;
};

struct HardwareAlarm
{
    int enable;
};

struct CellAlarm
{
    int cell_number = 0;
    int alm_status = 0;
    int alm_code = 0;
};

// used to send the balancing status to mini-pc
struct CellBalancingStatus
{
    int bid = 0;
    int cball[45] = {0};
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
    int addrCommand = 0;
    int alarmCommand = 0;
    int dataCollectionCommand = 0;
    int sleepCommand = 0;
};

struct AddressingCommand
{
    int exec = 0;
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

struct SleepCommand
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

struct FrameWrite
{
    int bid = 0;
    int write = 0;
    String frameName = "";
};

struct RmsCodeWrite
{
    int write = 0;
    String rmsCode = "";
};

struct RmsRackSnWrite
{
    int write = 0;
    String rackSn = "";
};

struct CMSCodeWrite
{
    int bid = 0;
    int write = 0;
    String cmsCode = "";
};

struct BaseCodeWrite
{
    int bid = 0;
    int write = 0;
    String baseCode = "";
};

struct McuCodeWrite
{
    int bid = 0;
    int write = 0;
    String mcuCode = "";
};

struct SiteLocationWrite
{
    int bid = 0;
    int write = 0;
    String siteLocation = "";
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

struct OtaParameter
{
    bool isOtaUpdate = 0;
    String server = "";
    int port = 80;
    String path = "";
};

struct AddressingStatus
{
    int numOfDevice = 0;
    int deviceAddressList[8] = {0};
    int status = 0;
};

#endif