#ifndef TALISRS485_TYPE_DEF_H
#define TALISRS485_TYPE_DEF_H

#include <stdint.h>
#include <stddef.h>
#include <cstdint>

namespace TalisRS485 {

enum Error : uint8_t {
    SUCCESS = 0x00,
    ERROR = 0x01,
    BUFFER_OVF = 0x02,
    NO_TERMINATE_CHARACTER = 0x03,
    IN_PAUSED_STATE = 0x04,
    TIMEOUT = 0xE0,
    REQUEST_QUEUE_FULL = 0xE8
};

enum RequestType : uint8_t {
    VCELL = 0,
    TEMP = 1,
    VPACK = 2,
    CMSSTATUS = 3,
    CMSREADBALANCINGSTATUS = 4,
    LED = 5,
    CMSINFO = 6,
    CMSFRAMEWRITE = 7,
    CMSCODEWRITE = 8,
    CMSBASECODEWRITE = 9,
    CMSMCUCODEWRITE = 10,
    CMSSITELOCATIONWRITE = 11,
    BALANCINGWRITE = 12,
    SHUTDOWN = 13,
    WAKEUP = 14,
    RESTART = 15,
    ADDRESS = 16
};

};

#endif