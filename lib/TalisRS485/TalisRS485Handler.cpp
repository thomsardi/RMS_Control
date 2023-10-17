#include "TalisRS485Handler.h"

TalisRS485Handler::TalisRS485Handler()
{
  _txMsgQueue.reserve(10);
  _rxMsgQueue.reserve(10);
}

void TalisRS485Handler::begin(HardwareSerial *hwSerial, bool isAscii, char terminateCharacter)
{
  _hwSerial = hwSerial;
  _messagePacketInterval = TalisRS485Utils::calculateInterval(_hwSerial->baudRate());
  _isAscii = isAscii;
  _terminateCharacter = terminateCharacter;
  ESP_LOGV(_TAG, "silent interval : %d\n", _messagePacketInterval);
  // _messagePacketInterval = 20000;
}

void TalisRS485Handler::setTimeout(int timeout)
{
  _timeout = timeout;
}

void TalisRS485Handler::setASCIIMode()
{
  _isAscii = true;
}

void TalisRS485Handler::setRTUMode()
{
  _isAscii = false;
}

void TalisRS485Handler::setTerminateCharacter(char terminateCharacter)
{
  _terminateCharacter = terminateCharacter;
}

bool TalisRS485Handler::onDataHandler(TalisRS485OnData handler)
{
  _onData = handler;
  return 1;
}

bool TalisRS485Handler::onErrorHandler(TalisRS485OnError handler)
{
  _onError = handler;
  return 1;
} 

bool TalisRS485Handler::onResponseHandler(TalisRS485OnResponse handler)
{
  _onResponse = handler;
  return 1;
}

bool TalisRS485Handler::isTxQueueEmpty()
{
  return _txMsgQueue.empty();
}

bool TalisRS485Handler::isRxQueueEmpty()
{
  return _rxMsgQueue.empty();
}

void TalisRS485Handler::pause()
{
  _isPaused = true;
}

void TalisRS485Handler::resume()
{
  _isPaused = false;
}

void TalisRS485Handler::reset()
{
  _isReset = true;
}

bool TalisRS485Handler::isPaused()
{
  return _isPaused;
}

void TalisRS485Handler::handle()
{
  if (_isReset)
  {
    _txMsgQueue.clear();
    _rxMsgQueue.clear();
    _isPaused = true;
  }
  
  if (!_isPaused)
  {
    if (!_txMsgQueue.empty())
    {
      TalisRS485TxMessage txMsg = _txMsgQueue.front();
      send(txMsg);
      TalisRS485RxMessage readMsg = receive(_timeout);
      if (readMsg.error == TalisRS485::Error::SUCCESS)
      {
        readMsg.id = txMsg.id;
        readMsg.requestCode = txMsg.requestCode;
        readMsg.token = txMsg.token;
        if (_onData != nullptr)
        {
          _onData(readMsg, readMsg.token);
        }
      }
      else
      {
        readMsg.token = txMsg.token;
        if (_onError != nullptr)
        {
          _onError(static_cast<TalisRS485::Error>(readMsg.error), readMsg.token);
        }
      }
      _txMsgQueue.erase(_txMsgQueue.begin());
    }
  }
}

bool TalisRS485Handler::handleAddressing()
{
  TalisRS485RxMessage rxMsg = receive(_timeout);
  if (rxMsg.error == TalisRS485::Error::SUCCESS)
  {
    _onData(rxMsg, 2000);
    return 1;
  }
  else
  {
    _onError(static_cast<TalisRS485::Error>(rxMsg.error), 2000);
    return 0;
  }
}

TalisRS485::Error TalisRS485Handler::addRequest(TalisRS485TxMessage m)
{
    if (_isPaused)
    {
      return TalisRS485::Error::IN_PAUSED_STATE;
    }
    
    if (_txMsgQueue.size() >= 10)
    {
        return TalisRS485::Error::REQUEST_QUEUE_FULL;    
    }
    _txMsgQueue.push_back(m);
    return TalisRS485::Error::SUCCESS;
}

TalisRS485RxMessage TalisRS485Handler::receive(int timeout)
{
    uint8_t buffer[512];
    uint8_t bufferPtr = 0;
    int maxBlockSize = sizeof(buffer) / sizeof(buffer[0]);
    TalisRS485RxMessage msg;
    msg.token = 9999;
    enum STATES : uint8_t { WAIT_DATA = 0, IN_PACKET, DATA_READ, FINISHED };

    uint8_t state;
    state = WAIT_DATA;
    unsigned long lastMicros;
    unsigned long TimeOut = millis();
    unsigned long lastMillis = millis();
    // interval tracker 
    lastMicros = micros();
    int b;
    // ESP_LOGV(_TAG, "timeout value : %d\n", _timeout);

    while (state != FINISHED) {
      switch (state) {
      // WAIT_DATA: await first data byte, but watch timeout
      case WAIT_DATA:
        // Blindly try to read a byte
        b = _hwSerial->read();
        // Did we get one?
        if (b >= 0) {
          // Yes. Note the time.
          // ESP_LOGV(_TAG,"read incoming data\n");
          buffer[bufferPtr++] = b; //assignment first and then increment the index / pointer
          state = IN_PACKET;
          lastMicros = micros();
          lastMillis = millis();
          
        } else {
          // No, we had no byte. Just check the timeout period
          if (millis() - TimeOut >= timeout) {
            // ESP_LOGV(_TAG,"time difference : %d\n", (millis() - TimeOut));
            // ESP_LOGV(_TAG, "Timeout!");
            msg.error = TalisRS485::Error::TIMEOUT;
            state = FINISHED;
            
          }
          // delay(1);
        }
        break;
      // IN_PACKET: read data until a gap of at least _interval time passed without another byte arriving
      case IN_PACKET:
        // tight loop until finished reading or error
        while (state == IN_PACKET) {
          // Is there a byte?
          while (_hwSerial->available()) {
            // Yes, collect it
            buffer[bufferPtr++] = _hwSerial->read();

            // Mark time of last byte
            lastMicros = micros();
            lastMillis = millis();
            // Buffer full?
            if (bufferPtr >= maxBlockSize) {
              // Yes. Something fishy here - bail out!
              msg.error = TalisRS485::Error::BUFFER_OVF;
              state = FINISHED;
              // Serial.println("buffer ovf");
              break;
            }
          } 
          // No more byte read
          if (state == IN_PACKET) {
            // Are we past the interval gap?
            
            if (_isAscii)
            {
              if (buffer[bufferPtr-1] == _terminateCharacter)
              {
                // ESP_LOGV(_TAG, "Terminate character found");
                state = DATA_READ;
                break;
              }
              if (millis() - lastMillis >= timeout) {
                // Yes, terminate reading
                // ESP_LOGV(_TAG, "No Terminate character found, timeout!");
                state = FINISHED;
                msg.error = TalisRS485::Error::NO_TERMINATE_CHARACTER;
                break;
              }
            }
            else            
            {
              if (micros() - lastMicros >= _messagePacketInterval) {
                // Yes, terminate reading
                // ESP_LOGV(_TAG, "Silent interval");
                state = DATA_READ;
                break;
              }
            }
          }
        }
        break;
      // DATA_READ: successfully gathered some data. Prepare return object.
      case DATA_READ:
        // Did we get a sensible buffer length?
        // LOG_V("%c/", (const char)caller);
        // HEXDUMP_V("Raw buffer received", buffer, bufferPtr);
        // ESP_LOGV(_TAG, "Copy data to buffer");
        msg.writeBuffer(buffer, bufferPtr);
        msg.error = TalisRS485::Error::SUCCESS;
        state = FINISHED;
        break;
      // FINISHED: we are done, clean up.
      case FINISHED:
        // CLear serial buffer in case something is left trailing
        // May happen with servers too slow!
        ESP_LOGV(_TAG, "Finish");
        while (_hwSerial->available()) _hwSerial->read();
        // Serial.println("finish");
        break;
      }
    }
    return msg;
}

// send: send a message via Serial, watching interval times - including CRC!
void TalisRS485Handler::send(const TalisRS485TxMessage &msg) {
    // Clear serial buffers
    while (_hwSerial->available()) _hwSerial->read();
    
    for (size_t i = 0; i < msg.dataLength; i++)
    {
      _hwSerial->write(msg.txData[i]);
    }

    ESP_LOG_BUFFER_CHAR(_TAG, msg.txData, msg.dataLength);
}

TalisRS485Handler::~TalisRS485Handler()
{

}