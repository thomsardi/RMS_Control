#include <Arduino.h>

struct CellData
{
    String frameName = "unspecified";
    int bid = 0;
    int vcell[45] = {0};
    int32_t temp[9] = {0};
    int32_t pack[3] = {0};
    int status = 0;
};

struct RMSInfo
{
    String p_code = "";
    String ver = "";
    String ip = "";
    String mac = "";
    String deviceTypeName = "";
};

struct CMSInfo
{
    String frameName = "unspecified";
    int bid = 0;
    String p_code = "";
    String ver = "";
    String chip = "";
};


struct AlarmParam
{
    int vcell_max = 0;
    int vcell_min = 0;
    int32_t temp_max = 0;
    int32_t temp_min = 0;
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

struct FrameWrite
{
    int bid = 0;
    int write = 0;
    String frameName = "";
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