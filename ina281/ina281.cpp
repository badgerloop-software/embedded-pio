#include "ina281.h"

/*
    Initialize I2C bus member and parameters
*/
INA281Driver::INA281Driver(int analogPin, float resistance, float scaleFactor)
{
    this->analogPin = analogPin;
    this->resistance = resistance;
    this->scaleFactor = scaleFactor; 
}

/*
    Retrieve new current reading
*/
float INA281Driver::readCurrent()
{
    int valueRead;

    valueRead = analogRead(this->analogPin);
    

    float measuredVoltage = (float)valueRead * 5/1024;

    // voltage = measuredVoltage / scaleFactor
    // current = voltage / resistance
    float current = (measuredVoltage / scaleFactor) / resistance;

    return current;
}

/*
    Retrieve new voltage reading
*/
float INA281Driver::readVoltage()
{
    int valueRead;
    valueRead = analogRead(this->analogPin);

    float measuredVoltage = (float)valueRead * 5/1024;

    float voltage = measuredVoltage / scaleFactor;

    return voltage;
}
