#ifndef MODBUS_REGISTER_HANDLER_H
#define MODBUS_REGISTER_HANDLER_H

#include <Arduino.h>
#include <JsonData.h>
#include <ModbusMessage.h>
#include <CoilData.h>
#include <ModbusServerWiFi.h>
#include <Preferences.h>

union SystemStatus {
    struct Bits {
        uint16_t cellDiffAlarm : 1;
        uint16_t cellOvervoltage : 1;
        uint16_t cellUndervoltage : 1;
        uint16_t overtemperature : 1;
        uint16_t undertemperature : 1;
        uint16_t : 3;
        uint16_t condition : 1;
        uint16_t ready : 1;
        uint16_t : 6;
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

    void set(size_t index, uint16_t value)
    {
        if (index >= this->elementSize) 
        {
            return;
        }
        data[index] = value;
    }


};

struct SettingRegisters {
    
    uint16_t* data[41];
    size_t elementSize = 41;

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

    int32_t getInt(int i) {
        // Serial.println(*params.value.fields.temp_max_hi);
        // Serial.println(*params.value.data[5]);
        int32_t value;
        if ( i+1 >= this->elementSize || i < 0)
        {
            return *(this->data[0]);
        }
        value = (*(this->data[0])) << 16 + (*(this->data[1]));
        return value;
        
        // return *(this->params.value.data[i]);
    }

    bool getString(int i, String &s) {
        // Serial.println(*params.value.fields.temp_max_hi);
        // Serial.println(*params.value.data[5]);
        int32_t value;
        char buff[33];
        for (size_t j = 0; j < 16; j++)
        {
            if ( i+j >= this->elementSize || i < 0)
            {
                return 0;
            }
            uint16_t value = get(i);
            char first = value & 0xFF;
            char second = value >> 8;
            buff[j*2] = first;
            if (first == '\0')
            {
                String result(buff);
                s = result;
                return 1;
            }
            buff[(j*2) + 1] = second;
            if (second == '\0')
            {
                String result(buff);
                s = result;
                return 1;
            }
            
        }
        buff[33] = '\0';
        String result(buff);
        s = result;
        return 1;
        // return *(this->params.value.data[i]);
    }

    void set(size_t index, uint16_t value) 
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
    bool *data[11];
    size_t elementSize = 11;

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