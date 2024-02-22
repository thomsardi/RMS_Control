#ifndef TALISRS485_MESSAGE_H
#define TALISRS485_MESSAGE_H

#define TALIS_TX_BUFFER_SIZE 256
#define TALIS_RX_BUFFER_SIZE 512

#include <ArduinoJson.h>
#include "TalisRS485TypeDefs.h"

struct TalisRS485TxMessage
{
    uint32_t token;
    int id = 0;
    uint8_t requestCode;
    std::array<uint8_t, TALIS_TX_BUFFER_SIZE> txData;
    int dataLength = -1;

    bool writeBuffer(uint8_t *source, int length)
    {
        if (length >= TALIS_TX_BUFFER_SIZE)
        {
            return 0;
        }
        for (size_t i = 0; i < length; i++)
        {
            this->txData[i] = source[i];
        }
        this->dataLength = length;
        return 1;
    }

};

struct TalisRS485RxMessage {
    uint32_t token;
    int id = 0;
    uint8_t requestCode;
    uint8_t error;
    std::array<uint8_t, TALIS_RX_BUFFER_SIZE> rxData;
    int dataLength = -1;

    bool writeBuffer(uint8_t *source, int length)
    {
        if (length >= TALIS_RX_BUFFER_SIZE)
        {
            return 0;
        }
        for (size_t i = 0; i < length; i++)
        {
            this->rxData[i] = source[i];
        }
        this->dataLength = length;
        return 1;
    }

    

};

struct LedColor {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

class TalisRS485Message 
{
private :

public :
    TalisRS485Message();
    
    /**
     * Read Request
    */
    static bool createVcellRequest(TalisRS485TxMessage &txMsg);
    static bool createTemperatureRequest(TalisRS485TxMessage &txMsg);
    static bool createVpackRequest(TalisRS485TxMessage &txMsg);
    static bool createReadBalancingStatus(TalisRS485TxMessage &txMsg);
    static bool createCMSStatusRequest(TalisRS485TxMessage &txMsg);
    static bool createShutDownRequest(TalisRS485TxMessage &txMsg);
    static bool createWakeupRequest(TalisRS485TxMessage &txMsg);
    static bool createCMSInfoRequest(TalisRS485TxMessage &txMsg);
    static bool createCMSResetRequest(TalisRS485TxMessage &txMsg);

    /**
     * Write Request
    */
    static bool createCMSFrameWriteIdRequest(TalisRS485TxMessage &txMsg, String frameName);
    static bool createCMSCodeWriteRequest(TalisRS485TxMessage &txMsg, String cmsCode);
    static bool createCMSBaseCodeWriteRequest(TalisRS485TxMessage &txMsg, String baseCode);
    static bool createCMSMcuCodeWriteRequest(TalisRS485TxMessage &txMsg, String mcuCode);
    static bool createCMSSiteLocationWriteRequest(TalisRS485TxMessage &txMsg, String siteLocation);
    static bool createCMSWriteBalancingRequest(TalisRS485TxMessage &txMsg, const int cellCommand[], size_t length);
    static bool createCMSWriteLedRequest(TalisRS485TxMessage &txMsg, const LedColor ledColor[], size_t length);
    
};


#endif