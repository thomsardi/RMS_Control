#include "ModbusRegisterHandler.h"

ModbusRegisterHandler::ModbusRegisterHandler(const ModbusRegisterData &modbusRegisterData)
{
    _cellDataptr = modbusRegisterData.inputRegister.cellData;
    _cellDataSize = modbusRegisterData.inputRegister.cellDataSize;
    _otherInfo = modbusRegisterData.inputRegister.otherInfo;
    _settingRegisters = modbusRegisterData.holdingRegisters.settingRegisters;
    _mbusCoilData = modbusRegisterData.mbusCoil.mbusCoilData;
    _elementSize = sizeof(_memory) / sizeof(_memory[0]);
}

ModbusMessage ModbusRegisterHandler::handleReadCoils(const ModbusMessage &request)
{
    ModbusMessage response;
    // Request parameters are first coil and number of coils to read
    uint16_t start = 0;
    uint16_t numCoils = 0;
    request.get(2, start, numCoils);
    CoilData myCoils(_mbusCoilData->elementSize);

    int endAddr = start + numCoils;
    
    // Are the parameters valid?
    if (start + numCoils <= myCoils.coils()) {

        for (size_t i = start; i < endAddr; i++)
        {
            myCoils.set(i, *(_mbusCoilData->data[i]));
        }

        // Looks like it. Get the requested coils from our storage
        vector<uint8_t> coilset = myCoils.slice(start, numCoils);
        // Set up response according to the specs: serverID, function code, number of bytes to follow, packed coils
        response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)coilset.size(), coilset);
    } else {
        // Something was wrong with the parameters
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    }
    // Return the response
    return response;
}

ModbusMessage ModbusRegisterHandler::handleReadInputRegisters(const ModbusMessage &request)
{
    ModbusMessage response;
    uint16_t addr = 0;           // Start address
    uint16_t words = 0;          // # of words requested
    request.get(2, addr);        // read address from request
    request.get(4, words);       // read # of words from request

    int startAddr = addr;
    int endAddr = addr + words;

    if (addr != 9000) 
    {
        int group = addr / _blockSize;
        
        int maxAddr = _cellDataSize*_blockSize + _elementSize;
        int maxGroupAddr = group *_blockSize + _elementSize;
        
        // Address overflow?
        // Set up response
        response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
        if ((addr + words) > maxAddr || (addr + words) > maxGroupAddr || group > _cellDataSize) {
            
            // Yes - send respective error response
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
            Serial.println("Illegal Data Address");
        }
        else
        {
            
            uint16_t memory[_elementSize] = {0};
            
            for (size_t i = 0; i < 45; i++) //45 cell data
            {
                int idx = 0 + i;
                memory[idx] = _cellDataptr[group].vcell[i];
            }
            for (size_t i = 0; i < 9; i++) //9 temp data
            {
                int idx = 45 + (i*2);
                memory[idx] = _cellDataptr[group].temp[i] >> 16;
                memory[idx+1] = _cellDataptr[group].temp[i] & 0xffff;
            }
            for (size_t i = 0; i < 3; i++) //3 vpack data
            {
                int idx = 45 + 18 + (i*2);
                memory[idx] = _cellDataptr[group].pack[i] >> 16;
                memory[idx+1] = _cellDataptr[group].pack[i] & 0xffff;
            }
            memory[69] = _cellDataptr[group].packStatus.val;

            for (uint8_t i = startAddr; i < endAddr; ++i) //add all requested address
            {
                // send increasing data values
                response.add(memory[i]);
            }
            
        }
        
    }
    else 
    {      
        response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
        if ((addr + words) > (9000 + _otherInfo->elementSize)) {
            
            // Yes - send respective error response
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
            Serial.println("Illegal Data Address");
        }
        else
        {
            for (uint8_t i = startAddr; i < endAddr; ++i) //add all requested address
            {
                // send increasing data values
                // Serial.println((*_otherInfo)[i]);
                response.add((*_otherInfo).get(i));
            }
        }
    }
    return response;
}

ModbusMessage ModbusRegisterHandler::handleReadHoldingRegisters(const ModbusMessage &request)
{
    ModbusMessage response;
    uint16_t addr = 0;           // Start address
    uint16_t words = 0;          // # of words requested
    request.get(2, addr);        // read address from request
    request.get(4, words);       // read # of words from request
    
    int maxAddr = 0 + _settingRegisters->elementSize;
    int startAddr = addr;
    int endAddr = addr + words;
    // Address overflow?
    // Set up response
    response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
    if ((addr + words) > maxAddr) {
        
        // Yes - send respective error response
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
        Serial.println("Illegal Data Address");
    }
    else
    {
        for (uint8_t i = startAddr; i < endAddr; ++i) //add all requested address
        {
            // send increasing data values
            response.add((*_settingRegisters).get(i));
        }
        
    }
        
    return response;
}

ModbusMessage ModbusRegisterHandler::handleWriteCoil(ModbusMessage &request)
{
    // FC05: act on 0x05 requests - WRITE_COIL
    ModbusMessage response;
    // Request parameters are coil number and 0x0000 (OFF) or 0xFF00 (ON)
    uint16_t start = 0;
    uint16_t state = 0;
    request.get(2, start, state);

    // Is the coil number valid?
    if (start < _mbusCoilData->elementSize) {
        // Looks like it. Is the ON/OFF parameter correct?
        if (state == 0x0000 || state == 0xFF00) {
        // Yes. We can set the coil
        bool coilState = false;

        if (state == 0xFF00)
        {
            coilState = true;
        }

        if (_mbusCoilData->set(coilState, start)) {
            // All fine, coil was set.
            response = ECHO_RESPONSE;
            // Pull the trigger
            // coilTrigger = true;
        } else {
            // Setting the coil failed
            response.setError(request.getServerID(), request.getFunctionCode(), SERVER_DEVICE_FAILURE);
        }
        } else {
        // Wrong data parameter
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
        }
    } else {
        // Something was wrong with the coil number
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    }
    // Return the response
    return response;
}

ModbusMessage ModbusRegisterHandler::handleWriteMultipleRegisters(ModbusMessage &request)
{
    ModbusMessage response;
    uint16_t addr = 0;           // Start address
    uint16_t words = 0;          // # of words requested
    request.get(2, addr);        // read address from request
    request.get(4, words);       // read # of words from request

    int maxAddr = 0 + _settingRegisters->elementSize;
    int startAddr = addr;
    int endAddr = addr + words;

    Serial.println(addr);
    Serial.println(words);
    // Address overflow?
    // Set up response
    response.add(request.getServerID(), request.getFunctionCode());
    response.add(addr);
    response.add(words);
    
    if ((addr + words) > maxAddr) {
        
        // Yes - send respective error response
        Serial.println(addr+words);
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
        Serial.println("Illegal Data Address");
    }
    else
    {
        size_t index = 7;
        uint16_t value;
        for (uint8_t i = 0; i < words; ++i) //add all requested address
        {
            // send increasing data values
            index = request.get(index, value);
            (*_settingRegisters).set(value, startAddr + i);
        }
        
    }

    return response;
}

ModbusMessage ModbusRegisterHandler::handleWriteRegister(ModbusMessage &request)
{
    ModbusMessage response;
    uint16_t addr = 0;           // Start address
    uint16_t words = 1;          // # of words requested
    request.get(2, addr);        // register address from request

    int maxAddr = 0 + _settingRegisters->elementSize;

    // Address overflow?
    // Set up response
    response.add(request.getServerID(), request.getFunctionCode());
    response.add(addr);
    
    if ((addr + words) > maxAddr) {
        
        // Yes - send respective error response
        // Serial.println(addr+words);
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
        Serial.println("Illegal Data Address");
    }
    else
    {
        size_t index = 4;
        uint16_t value;
        for (uint8_t i = 0; i < words; ++i) //add all requested address
        {
            // send increasing data values
            index = request.get(index, value);
            (*_settingRegisters).set(value, addr);
            response.add(value);
            // Serial.println(value);
        }
        
    }

    return response;
}