#include "Utilities.h"

size_t Utilities::toDoubleChar(String s, uint16_t *buff, size_t length, bool swap)
{
    size_t stringLength = s.length();
    // Serial.println("string length : " + String(stringLength));
    size_t resultLength = 0;
    bool isEven = false;
    bool isLessCharacter = false;

    if (stringLength > length*2)
    {
        return resultLength;
    }
    else
    {
        isLessCharacter = true;
    }

    if (stringLength % 2 == 0)
    {
        // Serial.println("Even");
        isEven = true;
        resultLength = stringLength / 2;
    }
    else
    {
        // Serial.println("Odd");
        resultLength = (stringLength / 2) + 1;
    }

    for (size_t i = 0; i < length; i++)
    {
        buff[i] = 0;
    }

    for (size_t i = 0; i < resultLength; i++)
    {
        if (isEven)
        {
            if (swap)
            {
                buff[i] = Utilities::charConcat(s.charAt(i*2 + 1), s.charAt(i*2)); //place higher index to leftmost character
            }
            else
            {
                buff[i] = Utilities::charConcat(s.charAt(i*2), s.charAt(i*2 + 1)); //place lower index to leftmost character
            }
            
            if (isLessCharacter)
            {
                for (int j = resultLength; j < length; j++)
                {
                    buff[j] = 0; //Null terminator
                }
            }
        }
        else
        {
            if (i == (resultLength - 1))
            {
                if (swap)
                {
                    buff[i] = Utilities::charConcat('\0', s.charAt(i*2));
                }
                else
                {
                    buff[i] = Utilities::charConcat(s.charAt(i*2), '\0');
                }
                    
                
                if (isLessCharacter)
                {
                    for (int j = resultLength; j < length; j++)
                    {
                        buff[j] = 0; //Null terminator
                    }
                }
            }
            else
            {
                if (swap)
                {
                    buff[i] = Utilities::charConcat(s.charAt(i*2 + 1), s.charAt(i*2)); //place higher index to leftmost character
                }
                else
                {
                    buff[i] = Utilities::charConcat(s.charAt(i*2), s.charAt(i*2 + 1)); //place lower index to leftmost character
                }            
            }
        }
    }    
    return resultLength; 
}

uint16_t Utilities::charConcat(const char &first, const char &second)
{
    uint16_t result = 0;

    result = (first << 8) + second;

    return result;
}

uint16_t Utilities::swap16(uint16_t value)
{
    uint16_t result = 0;

    uint8_t first = value >> 8;
    uint8_t second = value & 0xff;

    result = (second << 8) + first;
    return result;
}

int Utilities::getBit(int pos, int data)
{
  if (pos > 7 & pos < 0)
  {
    return -1;
  }
  int temp = data >> pos;
  int result = temp & 0x01;
  return result;
}

