#include <Updater.h>

Updater::Updater()
{

}

void Updater::resetUpdater()
{
    resetUpdateStatus();
}


int Updater::isUpdate()
{
    int status = 0;
    status = checkDataCompleted();
    if (status)
    {
        // resetUpdateStatus();
        Serial.println("Data Completed");
    }
    return status;
}

void Updater::updateVcell(bool isVcellNormal)
{
    _isVcellNormal = isVcellNormal;
    _isTempUpdated = false;
    _isVpackUpdated = false;
    _isStatusUpdated = false;
    _isVcellUpdated = true;
}

void Updater::updateTemp(bool isTempNormal)
{
    _isTempNormal = isTempNormal;
    _isTempUpdated = true;
}

void Updater::updateVpack(bool isVpackNormal)
{
    _isVpackNormal = isVpackNormal;
    _isVpackUpdated = true;
}

void Updater::updateStatus()
{
    _isStatusUpdated = true;
}

void Updater::resetUpdateStatus()
{
    _isVcellUpdated = false;
    _isTempUpdated = false;
    _isVpackUpdated = false;
    _isStatusUpdated = false;
}

bool Updater::isDataNormal()
{
    return (_isVcellNormal && _isTempNormal);
}

int Updater::checkDataCompleted()
{
    int status = 0;
    if (_isVcellUpdated && _isTempUpdated && _isVpackUpdated && _isStatusUpdated)
    {
        status = 1;
    }
    return status;
}