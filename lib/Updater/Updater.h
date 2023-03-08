#ifndef UPDATER_H
#define UPDATER_H

#include <Arduino.h>

class Updater
{
    public:
        Updater();
        int isUpdate();
        void updateVcell(bool isVcellNormal);
        void updateTemp(bool isTempNormal);
        void updateVpack(bool isVpackNormal);
        void updateStatus();
        void resetUpdater();
        bool isDataNormal();
    private:
        void resetUpdateStatus();
        int checkDataCompleted();
        int _isUpdate;
        int _isVcellUpdated;
        int _isTempUpdated;
        int _isVpackUpdated;
        int _isStatusUpdated;
        bool _isVcellNormal;
        bool _isTempNormal;
        bool _isVpackNormal;
};

#endif