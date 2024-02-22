#include <LedAnimation.h>

LedAnimation::LedAnimation()
{

}

LedAnimation::LedAnimation(size_t groupNumber, size_t stringNumber, bool isLowerToHigher)
{
    _groupNumber = groupNumber;
    _stringNumber = stringNumber;
    _isLowerToHigher = isLowerToHigher;
    _isUp = true;
    if (_isLowerToHigher)
    {
        _currentGroup = 0; 
    }
    else
    {
        _currentGroup = groupNumber - 1;
    }
    _currentString = stringNumber - 1;
}

void LedAnimation::run()
{
    _isRun = true;
}

void LedAnimation::stop()
{
    _isRun = false;
}

bool LedAnimation::isRunning()
{
    return _isRun;
}

void LedAnimation::setLedGroupNumber(size_t groupNumber)
{
    if (groupNumber <= 0)
    {
        return;
    }

    if(_isLowerToHigher)
    {
        _groupNumber = groupNumber;
        _currentGroup = 0;
    }
    else
    {
        _groupNumber = groupNumber;
        _currentGroup = groupNumber - 1;
    }
    
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
    if(_isLowerToHigher)
    {
        _currentGroup = 0;
        _currentString = _stringNumber - 1;
    }
    else
    {
        _currentGroup = _groupNumber - 1;
        _currentString = _stringNumber - 1;
    }
    
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

void LedAnimation::update()
{
    if(!_isRun)
    {
        return;
    }
    if (_groupNumber < 0 || _stringNumber < 0)
    {
        return;
    }
    // Serial.println("Group : " + String(_currentGroup));
    // Serial.println("String : " + String(_currentString));
    _ledData.currentGroup = _currentGroup;
    _ledData.currentString = _currentString;
    _outputLedData.currentGroup = _ledData.currentGroup;
    _outputLedData.currentString = _ledData.currentString;
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
        _outputLedData.red[i] = _ledData.red[i];
        _outputLedData.green[i] = _ledData.green[i];
        _outputLedData.blue[i] = _ledData.blue[i];
    }

    if(_isUp)
    {
        _currentString--;
    }
    else
    {
        _currentString++;
    }

    if (_isUp) // animation direction is from bottom to top
    {
        if (_currentString < 0)
        {
            _currentString = _stringNumber - 1;
            if(_isLowerToHigher)
            {
                _currentGroup++;
                if (_currentGroup >= _groupNumber)
                {
                    _currentGroup = _groupNumber - 1;
                    _currentString = 0;
                    _isUp = false;
                }
            }
            else
            {
                _currentGroup--;
                if (_currentGroup < 0)
                {
                    _currentGroup = 0;
                    _currentString = 0;
                    _isUp = false;
                }
            }
            
            _isGroupChanged = true;
        }
    }
    else //animation direction is from top to bottom
    {
        if (_currentString > 7) //check if it is in the end of the led string
        {
            if(_isLowerToHigher)
            {
                _currentString = 0;
                _currentGroup--;
                if (_currentGroup < 0)
                {
                    _currentGroup = 0;
                    _currentString = _stringNumber - 1;
                    _isUp = true;
                }
            }
            else
            {
                _currentString = 0;
                _currentGroup++;
                if (_currentGroup >= _groupNumber)
                {
                    _currentGroup = _groupNumber - 1;
                    _currentString = _stringNumber - 1;
                    _isUp = true;
                }
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
}

LedData LedAnimation::getLed()
{
    return _outputLedData;
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