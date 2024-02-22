#include "TalisRS485Handler.h"

TalisRS485Handler::TalisRS485Handler()
{
  _txMsgQueue.reserve(10);
  _rxMsgQueue.reserve(10);
  esp_log_level_set(_TAG, ESP_LOG_INFO);
}

void TalisRS485Handler::enableLogOutput()
{
  esp_log_level_set(_TAG, ESP_LOG_INFO);
}

void TalisRS485Handler::disableLogOutput()
{
  esp_log_level_set(_TAG, ESP_LOG_NONE);
}

void TalisRS485Handler::begin(HardwareSerial *hwSerial, bool isAscii, char terminateCharacter)
{
  _hwSerial = hwSerial;
  _messagePacketInterval = TalisRS485Utils::calculateInterval(_hwSerial->baudRate());
  _isAscii = isAscii;
  _terminateCharacter = terminateCharacter;
  ESP_LOGI(_TAG, "silent interval : %d\n", _messagePacketInterval);
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

void TalisRS485Handler::updateOnError(int bid, std::map<int, CMSData> &cmsData)
{
  if (cmsData.find(bid) != cmsData.end())
  {
    cmsData[bid].errorCount++;
  }
}

void TalisRS485Handler::resetError(int bid, std::map<int, CMSData> &cmsData)
{
  if (cmsData.find(bid) != cmsData.end())
  {
    cmsData[bid].errorCount = 0;
  }
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

      /**
       * This is broadcast message, do not expect any response, just skip and move on
      */
      if (txMsg.id == 255)
      {
        send(txMsg);
        _txMsgQueue.erase(_txMsgQueue.begin());
        return;
      }

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
          TalisRS485ErrorMessage errMsg;
          errMsg.id = txMsg.id;
          errMsg.errorCode = readMsg.error;
          // _onError(static_cast<TalisRS485::Error>(readMsg.error), readMsg.token);
          _onError(errMsg, readMsg.token);
        }
      }
      _txMsgQueue.erase(_txMsgQueue.begin());
    }
  }
}

bool TalisRS485Handler::handleAddressing()
{
  TalisRS485RxMessage rxMsg = receive(_timeout);
  ESP_LOGI(_TAG, "handleAddressing id : %d\n", rxMsg.id);
  if (rxMsg.error == TalisRS485::Error::SUCCESS)
  {
    _onData(rxMsg, 2000);
    return 1;
  }
  else
  {
    TalisRS485ErrorMessage errMsg;
    errMsg.id = rxMsg.id;
    errMsg.errorCode = rxMsg.error;
    // _onError(static_cast<TalisRS485::Error>(rxMsg.error), 2000);
    _onError(errMsg, 2000);
    return 0;
  }
}

int TalisRS485Handler::pendingRequest()
{
  return _txMsgQueue.size();
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
            // ESP_LOGI(_TAG, "Timeout, no packet data received!");
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
              // ESP_LOGI(_TAG, "Buffer OVF");
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
                // ESP_LOGI(_TAG, "Terminate character found");
                state = DATA_READ;
                break;
              }
              if (millis() - lastMillis >= timeout) {
                // Yes, terminate reading
                // ESP_LOGI(_TAG, "No Terminate character found, timeout!");
                state = FINISHED;
                msg.error = TalisRS485::Error::NO_TERMINATE_CHARACTER;
                break;
              }
            }
            else            
            {
              if (micros() - lastMicros >= _messagePacketInterval) {
                // Yes, terminate reading
                ESP_LOGI(_TAG, "Silent interval");
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
        // ESP_LOGI(_TAG, "Copy data to buffer");
        msg.writeBuffer(buffer, bufferPtr);
        msg.error = TalisRS485::Error::SUCCESS;
        state = FINISHED;
        break;
      // FINISHED: we are done, clean up.
      case FINISHED:
        // CLear serial buffer in case something is left trailing
        // May happen with servers too slow!
        // ESP_LOGI(_TAG, "Finish");
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
    
    String output;
    for (size_t i = 0; i < msg.dataLength; i++)
    {
      _hwSerial->write(msg.txData[i]);
      output += (char)msg.txData[i];
    }
    ESP_LOGI(_TAG, "%s\n", output.c_str());
}

int TalisRS485Handler::readCell(String input, std::map<int, CMSData> &cmsData)
{
  DynamicJsonDocument docBattery(1024);
  DeserializationError error = deserializeJson(docBattery, input);
  int bid = -1;    
  if (error) 
  {
    ESP_LOGI(_TAG, "deserializeJson() failed: %s\n", error.c_str());
    return -1;
  }
  JsonObject object = docBattery.as<JsonObject>();
  if (!object.isNull())
  {        
    if (docBattery.containsKey("BID") && docBattery.containsKey("VCELL"))
    {
      bid = docBattery["BID"];
      if (bid < 0)
      {
        return -1;
      }
      cmsData[bid].bid = bid;
      JsonArray jsonArray = docBattery["VCELL"].as<JsonArray>();
      int arrSize = jsonArray.size();
      if (arrSize == cmsData[bid].vcell.size())
      {
        // ESP_LOGI(_TAG, "Vcell Reading Address : %d\n", bid);                
        for (int i = 0; i < arrSize; i++)
        {
          cmsData[bid].vcell[i] = docBattery["VCELL"][i];
          // ESP_LOGI(_TAG, "Vcell %d = %d\n", i, cmsData[bid].vcell[i]);
        }
        resetError(bid, cmsData);
        return bid;
      }
    }
    return -1;
  }
  return -1;
}

bool TalisRS485Handler::checkCell(CMSData &cmsData, const AlarmParam &alarmParam)
{
  int maxVcell = cmsData.vcell[0];
  int minVcell = cmsData.vcell[0];
  int diff;
  bool isAllDataNormal = false;
  bool flag = true;
  bool undervoltageFlag = false;
  bool overvoltageFlag = false;
  bool diffVoltageFlag = false;
  for (int c : cmsData.vcell)
  {
    if (c <= 200) //ignore the unconnected cell
    {
      isAllDataNormal = true;
    }
    else  
    {
      if (maxVcell < c) 
      {
        maxVcell = c;
      }
      
      if (minVcell > c) {
        minVcell = c;
      }

      diff = maxVcell - minVcell;

      if (c >= alarmParam.vcell_undervoltage && c <= alarmParam.vcell_overvoltage) 
      {    
        if (cmsData.packStatus.bits.cellUndervoltage)
        {
          if (c < alarmParam.vcell_reconnect)
          {
            isAllDataNormal = false;
            flag = false;
            undervoltageFlag = true;
          }
        }
      }
      else
      {
        if (c < alarmParam.vcell_undervoltage) 
        {
          cmsData.packStatus.bits.cellUndervoltage = 1;
          isAllDataNormal = false;
          flag = false;    
          undervoltageFlag = true;
        }
        
        if (c > alarmParam.vcell_overvoltage)
        {
          cmsData.packStatus.bits.cellOvervoltage = 1;
          overvoltageFlag = true;
        }
      }

      if (cmsData.packStatus.bits.cellDiffAlarm) 
      {
        if (diff > alarmParam.vcell_diff_reconnect) 
        {
          isAllDataNormal = false;
          flag = false;
          diffVoltageFlag = true;
          // break;
        }
      }
      else
      {
        if (diff > alarmParam.vcell_diff) 
        {
          cmsData.packStatus.bits.cellDiffAlarm = 1;
          isAllDataNormal = false;
          flag = false;
          diffVoltageFlag = true;
          // break;
        }
      } 
    }
  }

  // ESP_LOGI(_TAG, "Max Vcell : %d\n", maxVcell);
  // ESP_LOGI(_TAG, "Min Vcell : %d\n", minVcell);
  // ESP_LOGI(_TAG, "Difference : %d\n", diff);

  cmsData.packStatus.bits.cellDiffAlarm = diffVoltageFlag;
  cmsData.packStatus.bits.cellUndervoltage = undervoltageFlag;
  cmsData.packStatus.bits.cellOvervoltage = overvoltageFlag;
  isAllDataNormal = isAllDataNormal & flag;

  return isAllDataNormal;
  // updater[startIndex].updateVcell(isAllDataNormal);
}

int TalisRS485Handler::readTemperature(String input, std::map<int, CMSData> &cmsData)
{
  DynamicJsonDocument docBattery(1024);
  DeserializationError error = deserializeJson(docBattery, input);
  int bid = -1;    
  if (error) 
  {
    ESP_LOGI(_TAG, "deserializeJson() failed: %s\n", error.c_str());
    return -1;
  }
  JsonObject object = docBattery.as<JsonObject>();
  if (!object.isNull())
  {        
    if (docBattery.containsKey("BID") && docBattery.containsKey("TEMP"))
    {
      bid = docBattery["BID"];
      // Serial.println("Contain TEMP key value");
      JsonArray jsonArray = docBattery["TEMP"].as<JsonArray>();
      int arrSize = jsonArray.size();
      if (arrSize == cmsData[bid].temp.size())
      {
        // ESP_LOGI(_TAG, "Temperature Reading Address : %d\n", bid);
        for (int i = 0; i < arrSize; i++)
        {
          float storage = docBattery["TEMP"][i];
          int32_t temp = static_cast<int32_t> (storage * 1000);
          cmsData[bid].temp[i] = temp;
          // ESP_LOGI(_TAG, "Temperature %d = %d\n", i, cmsData[bid].temp[i]);
        }
        resetError(bid, cmsData);
        return bid;
      }
    }
    return -1;
  }
  return -1;
}

bool TalisRS485Handler::checkTemperature(CMSData &cmsData, const AlarmParam &alarmParam)
{
  bool isAllDataNormal = false;
  bool flag = true;
  bool undertemperatureFlag = false;
  bool overtemperatureFlag = false;
  int maxTemp = cmsData.temp[0];
  int minTemp = cmsData.temp[0];
  for (int32_t temperature : cmsData.temp)
  {
    if (maxTemp < temperature)
    {
        maxTemp = temperature;
    }

    if (minTemp > temperature)
    {
        minTemp = temperature;
    }
    
    if (maxTemp > alarmParam.temp_max)
    {
      cmsData.packStatus.bits.overtemperature = 1;
      overtemperatureFlag = true;
    }

    if (minTemp < alarmParam.temp_min)
    {
      cmsData.packStatus.bits.undertemperature;
      undertemperatureFlag = true;
    }

    if (temperature > alarmParam.temp_max || temperature < alarmParam.temp_min)
    {
      isAllDataNormal = false;
      flag = false;
      // break;
    }
    else
    {
      isAllDataNormal = true;
    }
  }

  // ESP_LOGI(_TAG, "Max temperature = %d\n", maxTemp);
  // ESP_LOGI(_TAG, "Min temperature = %d\n", minTemp);

  cmsData.packStatus.bits.undertemperature = undertemperatureFlag;
  cmsData.packStatus.bits.overtemperature = overtemperatureFlag;

  isAllDataNormal = isAllDataNormal & flag;
  return isAllDataNormal;
}

int TalisRS485Handler::readVpack(String input, std::map<int, CMSData> &cmsData)
{
  DynamicJsonDocument docBattery(1024);
  DeserializationError error = deserializeJson(docBattery, input);
  int bid = -1;    
  if (error) 
  {
    ESP_LOGI(_TAG, "deserializeJson() failed: %s\n", error.c_str());
    return -1;
  }
  JsonObject object = docBattery.as<JsonObject>();
  if (!object.isNull())
  {        
    if (docBattery.containsKey("BID") && docBattery.containsKey("VPACK"))
    {
      bid = docBattery["BID"];

      JsonArray jsonArray = docBattery["VPACK"].as<JsonArray>();
      int arrSize = jsonArray.size();
      // Serial.println("Arr Size = " + String(arrSize));
      if (arrSize >= 4)
      {
        // ESP_LOGI(_TAG, "Vpack Reading Address : %d\n", bid);
        for (int i = 0; i < 4; i++)
        {
          uint32_t packVoltage = docBattery["VPACK"][i];
          if (i != 0) // index 0 is for total vpack
          {
            cmsData[bid].pack[i-1] = packVoltage;
            // ESP_LOGI(_TAG, "Vpack %d = %d\n", i, cmsData[bid].pack[i-1]);
          }
          else
          {
            // ESP_LOGI(_TAG, "Vpack Total = %d\n", packVoltage);
          }
        }
        resetError(bid, cmsData);
        return bid;
      }
    }
    return -1;
  }
  return -1;
}

bool TalisRS485Handler::checkVpack(CMSData &cmsData)
{
  bool isAllDataNormal = false;

  for (int32_t vpackValue : cmsData.pack)
  {
    if (vpackValue > 0)
    {
      isAllDataNormal = true;
    }
    else
    {
      isAllDataNormal = false;
      break;
    }
  }

  return isAllDataNormal;
}

int TalisRS485Handler::readInfo(String input, std::map<int, CMSData> &cmsData)
{
  DynamicJsonDocument docBattery(1024);
  DeserializationError error = deserializeJson(docBattery, input);
  int bid = -1;    
  if (error) 
  {
    ESP_LOGI(_TAG, "deserializeJson() failed: %s\n", error.c_str());
    return -1;
  }
  JsonObject object = docBattery.as<JsonObject>();
  if (!object.isNull())
  {        
    if(docBattery.containsKey("bid"))
    {
      bid = docBattery["bid"];
    }
    else
    {
      return -1;
    }

    if (docBattery.containsKey("cms_code"))
    {
      cmsData[bid].frameName = docBattery["frame_name"].as<String>();
      cmsData[bid].bid = bid;
      cmsData[bid].cmsCodeName = docBattery["cms_code"].as<String>();
      cmsData[bid].baseCodeName = docBattery["base_code"].as<String>();
      cmsData[bid].mcuCodeName = docBattery["mcu_code"].as<String>();
      cmsData[bid].siteLocation = docBattery["site_location"].as<String>();
      cmsData[bid].ver = docBattery["ver"].as<String>();
      cmsData[bid].chip = docBattery["chip"].as<String>();
    }
    else
    {
      return -1;
    }
    resetError(bid, cmsData);
    return bid;
  }
  return -1;
}

int TalisRS485Handler::readStatus(String input, std::map<int, CMSData> &cmsData)
{
  DynamicJsonDocument docBattery(1024);
  DeserializationError error = deserializeJson(docBattery, input);
  int bid = -1;    
  if (error) 
  {
    ESP_LOGI(_TAG, "deserializeJson() failed: %s\n", error.c_str());
    return -1;
  }
  if(!docBattery.containsKey("WAKE_STATUS"))
  {
    // Serial.println("Does not contain WAKE_STATUS");
    return -1;
  }

  if(!docBattery.containsKey("DOOR_STATUS"))
  {
    // Serial.println("Does not contain WAKE_STATUS");
    return -1;
  }

  if(!docBattery.containsKey("BID"))
  {
    // Serial.println("Does not contain BID");
    return -1;
  }

  bid = docBattery["BID"];
  cmsData[bid].packStatus.bits.status = docBattery["WAKE_STATUS"];
  cmsData[bid].packStatus.bits.door = docBattery["DOOR_STATUS"];
  resetError(bid, cmsData);
  return bid;
}

int TalisRS485Handler::readBalancing(String input, std::map<int, CellBalanceState> &cellBalanceState, std::map<int, CMSData> &cmsData)
{
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, input);
  int bid = -1;    
  if (error) 
  {
    ESP_LOGI(_TAG, "deserializeJson() failed: %s\n", error.c_str());
    return -1;
  }

  if (!(doc.containsKey("RBAL1.1") && doc.containsKey("RBAL2.1") && doc.containsKey("RBAL3.1")))
  {
    return -1;
  }

  if (doc.containsKey("BID"))
  {
    bid = doc["BID"];
    cellBalanceState[bid].bid = bid;
  }
  else
  {
    return -1;
  }

  int rbal[3];
  rbal[0] = doc["RBAL1.1"];
  rbal[1] = doc["RBAL1.2"];
  rbal[2] = doc["RBAL1.3"];
  for (size_t i = 0; i < 5; i++)
  {
    cellBalanceState[bid].cball[i] = Utilities::getBit(i, rbal[0]);
  }
  for (size_t i = 0; i < 5; i++)
  {
    cellBalanceState[bid].cball[i+5] = Utilities::getBit(i, rbal[1]);
  }
  for (size_t i = 0; i < 5; i++)
  {
    cellBalanceState[bid].cball[i+10] = Utilities::getBit(i, rbal[2]);
  }

  rbal[0] = doc["RBAL2.1"];
  rbal[1] = doc["RBAL2.2"];
  rbal[2] = doc["RBAL2.3"];
  for (size_t i = 0; i < 5; i++)
  {
    cellBalanceState[bid].cball[i+15] = Utilities::getBit(i, rbal[0]);
  }
  for (size_t i = 0; i < 5; i++)
  {
    cellBalanceState[bid].cball[i+20] = Utilities::getBit(i, rbal[1]);
  }
  for (size_t i = 0; i < 5; i++)
  {
    cellBalanceState[bid].cball[i+25] = Utilities::getBit(i, rbal[2]);
  }

  rbal[0] = doc["RBAL3.1"];
  rbal[1] = doc["RBAL3.2"];
  rbal[2] = doc["RBAL3.3"];
  for (size_t i = 0; i < 5; i++)
  {
    cellBalanceState[bid].cball[i+30] = Utilities::getBit(i, rbal[0]);
  }
  for (size_t i = 0; i < 5; i++)
  {
    cellBalanceState[bid].cball[i+35] = Utilities::getBit(i, rbal[1]);
  }
  for (size_t i = 0; i < 5; i++)
  {
    cellBalanceState[bid].cball[i+40] = Utilities::getBit(i, rbal[2]);
  }
  resetError(bid, cmsData);
  return bid;
}

int TalisRS485Handler::readLed(String input, std::map<int, CMSData> &cmsData)
{
  DynamicJsonDocument docBattery(1024);
  DeserializationError error = deserializeJson(docBattery, input);
  int bid = -1;
  int status = 0;    
  if (error) 
  {
    ESP_LOGI(_TAG, "deserializeJson() failed: %s\n", error.c_str());
    return -1;
  }
  if (!docBattery.containsKey("LEDSET"))
  {
    return -1;
  }
  if (!docBattery.containsKey("STATUS"))
  {
    return -1;
  }
  JsonObject object = docBattery.as<JsonObject>();
  if (!object.isNull())
  {
    bid = docBattery["BID"];
    status = docBattery["STATUS"];
    if (status > 0)
    {
      resetError(bid, cmsData);
      return bid;
    }
  }
  return bid;
}

TalisRS485Handler::~TalisRS485Handler()
{

}