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
        resetUpdateStatus();
        Serial.println("Data Completed");
    }
    return status;
}

void Updater::updateVcell()
{
    if (_isVcellUpdated)
    {
        _isTempUpdated = false;
        _isVpackUpdated = false;
        _isWakeStatusUpdated = false;
    }
    _isVcellUpdated = true;
    Serial.println("Vcell Updated");
}

void Updater::updateTemp()
{
    if (_isTempUpdated)
    {
        _isVcellUpdated = false;
        _isVpackUpdated = false;
        _isWakeStatusUpdated = false;

    }
    _isTempUpdated = true;
    Serial.println("Temp Updated");
}

void Updater::updateVpack()
{
    if (_isVpackUpdated)
    {
        _isVcellUpdated = false;
        _isTempUpdated = false;
        _isWakeStatusUpdated = false;
    }
    _isVpackUpdated = true;
    Serial.println("Vpack Updated");
}

void Updater::updateWakeStatus()
{
    if (_isWakeStatusUpdated)
    {
        _isVcellUpdated = false;
        _isTempUpdated = false;
        _isVpackUpdated = false;
    }
    _isWakeStatusUpdated = true;
    Serial.println("Wake Updated");
}

void Updater::resetUpdateStatus()
{
    _isVcellUpdated = false;
    _isTempUpdated = false;
    _isVpackUpdated = false;
    _isWakeStatusUpdated = false;
}

int Updater::checkDataCompleted()
{
    int status = 0;
    if (_isVcellUpdated && _isTempUpdated && _isVpackUpdated && _isWakeStatusUpdated)
    {
        status = 1;
    }
    return status;
}