#include "Utilities.h"

size_t Utilities::toDoubleChar(String s, uint16_t *buff, size_t length)
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
            buff[i] = (s.charAt(i*2) << 8) + s.charAt((i*2) + 1);
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
                buff[i] = (s.charAt(i*2) << 8) + '\0';
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
                buff[i] = (s.charAt(i*2) << 8) + s.charAt((i*2) + 1);
            }
        }
    }    
    return resultLength; 
}