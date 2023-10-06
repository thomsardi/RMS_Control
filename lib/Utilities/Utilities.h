#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>

class Utilities {
    public :
        static size_t toDoubleChar(String s, uint16_t *buff, size_t length, bool swap = false);
        static uint16_t charConcat(const char &first, const char &second);
        static uint16_t swap16(uint16_t value);
        static int getBit(int pos, int data);
        template <typename T> static bool _getBit(int pos, T data);
        template <typename T> static void fillArray(T a[], size_t len, T value);
        template <typename T> static void fillArrayRandom(T a[], size_t len, T min, T max);
    private :

};

/**
 * Not yet implemented, do not call this method
 * @brief   extract bit from specified data type
 * @return  extracted bit value (1 or 0) and always 0 if error
 * 
*/
template <typename T>
bool Utilities::_getBit(int pos, T data)
{
  size_t bitsCount = sizeof(data) * 8;
  Serial.println("Bits count : " + String(bitsCount));
  if (pos >= bitsCount & pos < 0) // prevent to access out of bound
  {
    return 0;
  }
  int temp = data >> pos;
  bool result = temp & 0x01;
  return result;
};

/**
 * @brief fill array with specified value
*/
template <typename T>
void Utilities::fillArray(T a[], size_t len, T value)
{
    for (size_t i = 0; i < len; i++)
    {
        a[i] = value;
    }
};

template <typename T>
void Utilities::fillArrayRandom(T a[], size_t len, T min, T max)
{
    for (size_t i = 0; i < len; i++)
    {
        a[i] = random(min, max);
    }
};

#endif