#ifndef LEDANIMATION_H
#define LEDANIMATION_H

#define LEDANIMATION_DEBUG

#ifdef LEDANIMATION_DEBUG
    #define LOG_PRINT(x)    Serial.print(x)
    #define LOG_PRINTLN(x)  Serial.println(x)
#else
    #define LOG_PRINT(x)
    #define LOG_PRINTLN(x)
#endif

#include <LedData.h>

class LedAnimation {
    public :
        LedAnimation();
        LedAnimation(size_t groupNumber, size_t stringNumber, bool isFromBottom = true);
        LedData update();
        void setLedGroupNumber(size_t groupNumber = 0);
        void setLedStringNumber(size_t stringNumber = 0);
        void restart();
        void run();
        void stop();
        bool isRunning();
    private :
        void resetLedData();
        bool _isRun = false;
        size_t _groupNumber = -1;
        size_t _stringNumber = -1;
        int _currentGroup = -1;
        int _currentString = -1;
        bool _isUp = true;
        bool _isGroupChanged = false;
        bool _isFromBottom;
        LedData _ledData;
};


#endif