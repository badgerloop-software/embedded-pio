#include "ina281.h"

/*
    Initialize I2C bus member and parameters
*/
INA281Driver::INA281Driver(uint8_t addr, float resistance, float scaleFactor, int Digital_Analog)
{
    Wire.begin();
    this->pinAddr = pinAddr;
    this->resistance = resistance;
    this->scaleFactor = scaleFactor;
    this->Digital_Analog = Digital_Analog; 
}

/*
    Retrieve new current reading
*/
float INA281Driver::readCurrent()
{
    float valueRead;

    if (Digital_Analog == 1){
        valueRead = digitalRead(pinAddr);
    }
    else {
        valueRead = analogRead(pinAddr);
    }

    float measuredVoltage = valueRead * 3.3;

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
    float valueRead;

    if (Digital_Analog == 1){
        valueRead = digitalRead(pinAddr);
    }
    else {
        valueRead = analogRead(pinAddr);
    }

    float voltage = valueRead / scaleFactor;

    return voltage;
}
