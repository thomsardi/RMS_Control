#ifndef MODBUS_REGISTER_HANDLER_H
#define MODBUS_REGISTER_HANDLER_H

#include <Arduino.h>
#include <JsonData.h>
#include <ModbusMessage.h>

class ModbusRegisterHandler {
    public :
        ModbusRegisterHandler(CellData* cellData, size_t cellDataSize);
        ModbusMessage handleRequest(const ModbusMessage &request);
    private :
        size_t _cellDataSize;
        CellData *_cellDataptr;
        uint16_t _memory[69];
        int _blockSize = 1000;
        int _elementSize = 0;
};

#endif