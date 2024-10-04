#include "ina281.h"

/*
    Initialize I2C bus member and parameters
*/
INA281Driver::INA281Driver(uint8_t pinAddr, float resistance, float scaleFactor)
{
    this->pinAddr = pinAddr;
    this->resistance = resistance;
    this->scaleFactor = scaleFactor; 
}

/*
    Retrieve new current reading
*/
float INA281Driver::readCurrent()
{
    int valueRead;

    valueRead = analogRead(pinAddr);
    

    float measuredVoltage = (float)valueRead * 5/1023;

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
    valueRead = analogRead(pinAddr);

    float measuredVoltage = (float)valueRead * 5/1023;

    float voltage = measuredVoltage / scaleFactor;

    return voltage;
}
