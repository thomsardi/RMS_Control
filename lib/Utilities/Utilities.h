#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>

class Utilities {
    public :
        static size_t toDoubleChar(String s, uint16_t *buff, size_t length, bool swap = false);
        static uint16_t charConcat(const char &first, const char &second);
        static uint16_t swap16(uint16_t value);
    private :

};


#endif