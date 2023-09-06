#include "ModbusRegisterHandler.h"

ModbusRegisterHandler::ModbusRegisterHandler(CellData* cellData, size_t cellDataSize)
{
    _cellDataptr = cellData;
    _cellDataSize = cellDataSize;
    _elementSize = sizeof(_memory) / sizeof(_memory[0]);
}

ModbusMessage ModbusRegisterHandler::handleRequest(const ModbusMessage &request)
{
    ModbusMessage response;
    uint16_t addr = 0;           // Start address
    uint16_t words = 0;          // # of words requested
    request.get(2, addr);        // read address from request
    request.get(4, words);       // read # of words from request

    int group = addr / _blockSize;
    
    int maxAddr = _cellDataSize*_blockSize + _elementSize;
    int maxGroupAddr = group *_blockSize + _elementSize;
    // Serial.println(_blockSize);
    // Serial.println(_elementSize);
    // Serial.println(group);
    // Serial.println(maxAddr);
    // Serial.println(maxGroupAddr);
    // Serial.println(_cellDataptr[0].cmsCodeName);
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
        if (request.getFunctionCode() == READ_INPUT_REGISTER) 
        {
            uint16_t memory[_elementSize] = {0};
            // for (size_t i = 0; i < 45; i++)
            // {
            //     memory[i] = 3000 + (i*10);
            // }
            
            // for (size_t i = 0; i < 9; i++)
            // {
            //     int temperature = 27000 + (i*100);
            //     int idx = 45 + (i*2);
            //     memory[idx] = temperature >> 16;
            //     memory[idx+1] = temperature & 0xffff;
            // }

            // for (size_t i = 0; i < 3; i++)
            // {
            //     int pack = 32000 + (i*300);
            //     int idx = 45 + 18 + (i*2);
            //     memory[idx] = pack >> 16;
            //     memory[idx+1] = pack & 0xffff;
            // }
            
            for (size_t i = 0; i < 45; i++)
            {
                int idx = 0 + i;
                memory[idx] = _cellDataptr[group].vcell[i];
            }
            for (size_t i = 0; i < 9; i++)
            {
                int idx = 45 + (i*2);
                memory[idx] = _cellDataptr[group].temp[i] >> 16;
                memory[idx+1] = _cellDataptr[group].temp[i] & 0xffff;
            }
            for (size_t i = 0; i < 3; i++)
            {
                int idx = 45 + 18 + (i*2);
                memory[idx] = _cellDataptr[group].pack[i] >> 16;
                memory[idx+1] = _cellDataptr[group].pack[i] & 0xffff;
            }

            for (uint8_t i = 0; i < words; ++i) {
                // send increasing data values
                response.add(memory[i]);
            }
        }
    }
    

    return response;
}