#ifndef MODBUS_REGISTER_HANDLER_H
#define MODBUS_REGISTER_HANDLER_H

#include <Arduino.h>
#include <JsonData.h>
#include <ModbusMessage.h>
#include <CoilData.h>
#include <ModbusServerWiFi.h>

union SystemStatus {
    struct Bits {
        uint16_t cellDiffAlarm : 1;
        uint16_t cellOvervoltage : 1;
        uint16_t cellUndervoltage : 1;
        uint16_t overtemperature : 1;
        uint16_t undertemperature : 1;
        uint16_t : 3;
        uint16_t condition : 1;
        uint16_t relayOutput : 1;
        uint16_t ready : 1;
        uint16_t : 5;
    } bits;
    uint16_t val;
};

struct OtherInfo {

    uint16_t data[2];

    size_t elementSize = 2;

    uint16_t get(int i) 
    {
        if ( i >= this->elementSize || i < 0)
        {
            return (this->data[0]);
        }
        return (this->data[i]);
    }

    void set(uint16_t value, size_t index)
    {
        if (index >= this->elementSize) 
        {
            return;
        }
        data[index] = value;
    }


};

struct SettingRegisters {
    
    uint16_t* data[9];
    size_t elementSize = 9;

    uint16_t get(int i) {
        // Serial.println(*params.value.fields.temp_max_hi);
        // Serial.println(*params.value.data[5]);
        if ( i >= this->elementSize || i < 0)
        {
            return *(this->data[0]);
        }
        return *(this->data[i]);
        // return *(this->params.value.data[i]);
    }

    void set(uint16_t value, size_t index) 
    {
        if (index >= this->elementSize) 
        {
            return;
        }
        *data[index] = value;
    }

    void link(uint16_t *sourceAddr, size_t index)
    {
        if (index >= this->elementSize)
        {
            return;
        }
        this->data[index] = sourceAddr;
    }
};

struct MbusCoilData {
    bool *data[8];
    size_t elementSize = 8;

    bool get(int i) {
        // Serial.println(*params.value.fields.temp_max_hi);
        // Serial.println(*params.value.data[5]);
        if ( i >= this->elementSize || i < 0)
        {
            return *(this->data[0]);
        }
        return *(this->data[i]);
        // return *(this->params.value.data[i]);
    }

    bool set(bool value, size_t index) 
    {
        if (index >= this->elementSize) 
        {
            return 0;
        }
        *data[index] = value;
        return 1;
    }

    void link(bool *sourceAddr, size_t index)
    {
        if (index >= this->elementSize)
        {
            return;
        }
        this->data[index] = sourceAddr;
    } 

};

struct ModbusRegisterData
{
    struct InputRegisters {
        CellData* cellData = nullptr;
        size_t cellDataSize = 0;
        OtherInfo* otherInfo;
    } inputRegister;

    struct HoldingRegisters {
        SettingRegisters *settingRegisters = nullptr;
    } holdingRegisters;

    struct Coils {
        MbusCoilData *mbusCoilData;
    } mbusCoil;
};
class ModbusRegisterHandler {
    public :
        ModbusRegisterHandler(const ModbusRegisterData &modbusRegisterData);
        ModbusMessage handleReadCoils(const ModbusMessage &request);
        ModbusMessage handleReadInputRegisters(const ModbusMessage &request);
        ModbusMessage handleReadHoldingRegisters(const ModbusMessage &request);
        ModbusMessage handleWriteCoil(ModbusMessage &request);
        ModbusMessage handleWriteMultipleRegisters(ModbusMessage &request);
        ModbusMessage handleWriteRegister(ModbusMessage &request);
    private :
        size_t _cellDataSize;
        CellData *_cellDataptr;
        OtherInfo *_otherInfo;
        MbusCoilData *_mbusCoilData;
        SettingRegisters *_settingRegisters;
        uint16_t _memory[70];
        int _blockSize = 1000;
        int _elementSize = 0;
};

#endif