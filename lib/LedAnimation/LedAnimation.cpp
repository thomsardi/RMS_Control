#include <LedAnimation.h>

LedAnimation::LedAnimation()
{

}

LedAnimation::LedAnimation(size_t groupNumber, size_t stringNumber)
{
    _groupNumber = groupNumber;
    _stringNumber= stringNumber;
    _currentGroup = groupNumber - 1;
    _currentString = stringNumber - 1;
}

void LedAnimation::setLedGroupNumber(size_t groupNumber)
{
    if (groupNumber <= 0)
    {
        return;
    }
    _groupNumber = groupNumber;
    _currentGroup = groupNumber - 1;
}

void LedAnimation::setLedStringNumber(size_t stringNumber)
{
    if (stringNumber <= 0)
    {
        return;
    }
    _stringNumber = stringNumber;
    _currentString = stringNumber - 1;
}

void LedAnimation::restart()
{
    _currentGroup = _groupNumber - 1;
    _currentString = _stringNumber - 1;
    _isUp = true;
    _isGroupChanged = false;
    _ledData.currentGroup = _currentGroup;
    _ledData.currentString = _currentString;
    for (size_t i = 0; i < _stringNumber; i++)
    {
        _ledData.red[i] = 0;
        _ledData.green[i] = 0;
        _ledData.blue[i] = 0;
    }
    
}

LedData LedAnimation::update()
{
    LedData ledData;
    if (_groupNumber < 0 || _stringNumber < 0)
    {
        return ledData;
    }
    _ledData.currentGroup = _currentGroup;
    _ledData.currentString = _currentString;
    ledData.currentGroup = _ledData.currentGroup;
    ledData.currentString = _ledData.currentString;
    // Serial.println("Current Group : " + String(_currentGroup));
    // Serial.println("Current String : " + String(_currentString));
    for (size_t i = 0; i < _stringNumber; i++)
    {
        if (i == _currentString)
        {
            if(_isUp)
            {
                _ledData.red[i] = 0;
                _ledData.green[i] = 255;
                _ledData.blue[i] = 0;
            }
            else
            {
                _ledData.red[i] = 0;
                _ledData.green[i] = 0;
                _ledData.blue[i] = 0;
            }
        }
        ledData.red[i] = _ledData.red[i];
        ledData.green[i] = _ledData.green[i];
        ledData.blue[i] = _ledData.blue[i];
    }

    if(_isUp)
    {
        _currentString--;
    }
    else
    {
        _currentString++;
    }
    
    
    if(_isUp)
    {
        if (_currentString < 0)
        {
            _currentString = _stringNumber - 1;
            _currentGroup--;
            if (_currentGroup < 0)
            {
                _currentGroup = 0;
                _currentString = 0;
                _isUp = false;
            }
            _isGroupChanged = true;
        }
    }
    else
    {
        if (_currentString > 7)
        {
            _currentString = 0;
            _currentGroup++;
            if (_currentGroup >= _groupNumber)
            {
                _currentGroup = _groupNumber - 1;
                _currentString = _stringNumber - 1;
                _isUp = true;
            }
            _isGroupChanged = true;
        }
    }

    if(_isUp)
    {
        if(_isGroupChanged)
        {
            // Serial.println("Is Up Group Changed");
            resetLedData();
            _isGroupChanged = false;
        }
    }
    else
    {
        if(_isGroupChanged)
        {
            // Serial.println("Is Down Group Changed");
            resetLedData();
            _isGroupChanged = false;
        }
    }
    return ledData;
}

void LedAnimation::resetLedData()
{
    for (size_t i = 0; i < _stringNumber; i++)
    {
        if(_isUp)
        {
            // Serial.println("Zero Led");
            _ledData.red[i] = 0;
            _ledData.green[i] = 0;
            _ledData.blue[i] = 0;
        }
        else
        {
            // Serial.println("Green Led");
            _ledData.red[i] = 0;
            _ledData.green[i] = 255;
            _ledData.blue[i] = 0;
        }
        
    }
    
}