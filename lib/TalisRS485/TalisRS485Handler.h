#ifndef TALISRS485_HANDLER_H
#define TALISRS485_HANDLER_H

#include <Arduino.h>
#include <vector>
#include "TalisRS485Message.h"
#include "TalisRS485Utils.h"

typedef std::function<void(TalisRS485RxMessage msg, uint32_t token)> TalisRS485OnData;
typedef std::function<void(TalisRS485::Error errorCode, uint32_t token)> TalisRS485OnError;
typedef std::function<void(TalisRS485RxMessage msg, uint32_t token)> TalisRS485OnResponse;

class TalisRS485Handler
{
private:
    /* data */
    const char* _TAG = "TalisRS485Handler.h";
    HardwareSerial* _hwSerial = nullptr;
    TalisRS485OnData _onData = nullptr;
    TalisRS485OnError _onError = nullptr;
    TalisRS485OnResponse _onResponse = nullptr;
    int _timeout = 3000;
    int _messagePacketInterval = 1750;
    char _terminateCharacter = '\n';
    bool _isAscii = false;
    bool _isPaused = false;
    bool _isReset = false;
    std::vector<TalisRS485TxMessage> _txMsgQueue;
    std::vector<TalisRS485RxMessage> _rxMsgQueue;

public:
    TalisRS485Handler();
    void pause();
    void resume();
    void reset();
    void handle();
    bool handleAddressing();
    TalisRS485RxMessage receive(int timeout);
    void send(const TalisRS485TxMessage &msg);
    void begin(HardwareSerial *hwSerial, bool isAscii = 0, char terminateCharacter = '\n');
    void setTimeout(int timeout = 3000);
    void setASCIIMode();
    void setRTUMode();
    void setTerminateCharacter(char terminateCharacter);
    bool onDataHandler(TalisRS485OnData handler);   // Accept onData handler 
    bool onErrorHandler(TalisRS485OnError handler); // Accept onError handler 
    bool onResponseHandler(TalisRS485OnResponse handler); // Accept onResponse handler 
    bool isTxQueueEmpty();
    bool isRxQueueEmpty();
    bool isPaused();
    TalisRS485::Error addRequest(TalisRS485TxMessage m);

    ~TalisRS485Handler();
};



#endif