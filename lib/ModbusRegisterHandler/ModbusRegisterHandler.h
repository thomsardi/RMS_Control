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

    public :
        uint16_t data[2];

        size_t elementSize = 2;

        uint16_t get(int i) 
        {
            if ( i >= this->elementSize || i < 0)
            {
                return 0;
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

        int32_t getInt(int index) 
        {
            int32_t value;
            if ( index+1 >= this->elementSize || index < 0)
            {
                return this->defaultValue;
            }
            value = (this->data[index] << 16) + (this->data[index+1]);
            return value;
        }

        bool getBulk(int index, uint16_t *buff, size_t length) 
        {
            if (index + length >= this->elementSize || index < 0) 
            {
                return 0;
            }

            for (size_t i = 0; i < length; i++)
            {
                buff[i] = data[index + i];
            }
            return 1;
        } 

        bool setBulk(size_t index, uint16_t *buff, size_t length) 
        {
            if (index + length >= this->elementSize) 
            {
                return 0;
            }

            for (size_t i = 0; i < length; i++)
            {
                data[index + i] = buff[i];
            }
            return 1;
        }

    private :
        uint16_t defaultValue = 0;

};

struct SettingRegisters {
    public :
        uint16_t* data[41];
        size_t elementSize = 41;

        SettingRegisters()
        {
            for (size_t i = 0; i < this->elementSize; i++)
            {
                link(&(this->defaultValue), i);
            }
        }

        uint16_t get(int index) {
            if ( index >= this->elementSize || index < 0)
            {
                return this->defaultValue;
            }
            return *(this->data[index]);
        }

        int32_t getInt(int index) {
            int32_t value;
            if ( index+1 >= this->elementSize || index < 0)
            {
                return this->defaultValue;
            }
            value = ((*(this->data[index])) << 16) + (*(this->data[index+1]));
            return value;
        }

        bool getBulk(int index, uint16_t *buff, size_t length) {
            if (index + length >= this->elementSize || index < 0) 
            {
                return 0;
            }

            for (size_t i = 0; i < length; i++)
            {
                buff[i] = *data[index + i];
            }
            return 1;
        } 

        static size_t stringToDoubleChar(String s, uint16_t *doubleChar, size_t length)
        {
            size_t stringLength = s.length();
            Serial.println("string length : " + String(stringLength));
            size_t resultLength = 0;
            bool isEven = false;
            
            if (stringLength > length*2)
            {
                return resultLength;
            }

            if (stringLength % 2 == 0)
            {
                Serial.println("Even");
                isEven = true;
                resultLength = stringLength / 2;
            }
            else
            {
                Serial.println("Odd");
                resultLength = (stringLength / 2) + 1;
            }

            for (size_t i = 0; i < resultLength; i++)
            {
                if (isEven)
                {
                    doubleChar[i] = (s.charAt(i*2) << 8) + s.charAt((i*2) + 1);
                }
                else
                {
                    if (i == (resultLength - 1))
                    {
                        doubleChar[i] = (s.charAt(i*2) << 8) + '\0';
                    }
                    else
                    {
                        doubleChar[i] = (s.charAt(i*2) << 8) + s.charAt((i*2) + 1);
                    }
                }
            }
            return resultLength; 
        }

        bool set(size_t index, uint16_t value) 
        {
            if (index >= this->elementSize) 
            {
                return 0;
            }
            *data[index] = value;
            return 1;
        }

        bool setBulk(size_t index, uint16_t *buff, size_t length) 
        {
            if (index + length >= this->elementSize) 
            {
                return 0;
            }

            for (size_t i = 0; i < length; i++)
            {
                *data[index + i] = buff[i];
            }
            return 1;
        }

        bool link(uint16_t *sourceAddr, size_t index)
        {
            if (index >= this->elementSize)
            {
                return 0;
            }
            this->data[index] = sourceAddr;
            return 1;
        }

        void unlinkAll()
        {
            for (size_t i = 0; i < this->elementSize; i++)
            {
                link(&(this->defaultValue), i);
            }
        }

    private :
        uint16_t defaultValue = 0;
};

struct MbusCoilData {
    public :
        bool *data[11];
        size_t elementSize = 11;

        MbusCoilData()
        {
            for (size_t i = 0; i < this->elementSize; i++)
            {
                link(&(this->defaultValue), i);
            }
        }

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

        bool getBulk(int index, bool *buff, size_t length) {
            if (index + length >= this->elementSize || index < 0) 
            {
                return 0;
            }

            for (size_t i = 0; i < length; i++)
            {
                buff[i] = *data[index + i];
            }
            return 1;
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

        bool setBulk(size_t index, bool *buff, size_t length) 
        {
            if (index + length >= this->elementSize) 
            {
                return 0;
            }

            for (size_t i = 0; i < length; i++)
            {
                *data[index + i] = buff[i];
            }
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

        void unlinkAll()
        {
            for (size_t i = 0; i < this->elementSize; i++)
            {
                link(&(this->defaultValue), i);
            }
        }

    private :
        bool defaultValue = 0;

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