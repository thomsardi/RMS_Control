#ifndef TALISRS485_UTILS_H
#define TALISRS485_UTILS_H

#include <stdint.h>
#include "Logging.h"

class TalisRS485Utils
{
private:
    /* data */
public:
    TalisRS485Utils(/* args */);
    static uint32_t calculateInterval(uint32_t baudRate);
    ~TalisRS485Utils();
};

#endif