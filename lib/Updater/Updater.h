#ifndef UPDATER_H
#define UPDATER_H

#include <Arduino.h>

class Updater
{
    public:
        Updater();
        int isUpdate();
        void updateVcell();
        void updateTemp();
        void updateVpack();
        void updateWakeStatus();
        void resetUpdater();
    private:
        void resetUpdateStatus();
        int checkDataCompleted();
        int _isUpdate;
        int _isVcellUpdated;
        int _isTempUpdated;
        int _isVpackUpdated;
        int _isWakeStatusUpdated;
};

#endif