#ifndef __THERMISTOR_H__
#define __THERMISTOR_H__

#include <Wire.h>

/*
    Thermistor voltage reading is converted to temperature in a two step process

    1. Thermistor voltage must be converted into thermistor resistance.
    This is done by solving the voltage divider equation (VCC and other resistor R are 
    fixed by hardware) for the thermistor voltage

    Measured Voltage = VCC * (Thermistor_Resistance) / (Thermistor_Resistance + R)

    2. Thermistor resistance is converted to temperature through a best fit 
    log curve built using the specific thermistor model's electrical characteristics.
    This will involve a pair of magic numbers (mult_const and add_const) generated by
    fitting a curve to a series of (resistance, temperature) points provided by manufacturer

    Temperature = mult_const * log(Thermistor Resistance) + add_const
*/

typedef struct{
    float mult_const;
    float add_const;
} Thermistor_Constants;

const Thermistor_Constants NCP21XM472J03RA_Constants = {
    .mult_const = -27.06,
    .add_const = 253.11
};

class Thermistor{
    private:
    float mult_const;
    float add_const;
    float vcc;
    float R;
    int thermVoltPin;

    public:
    /*
        Constructor expects at least 2 arguments
        constants: Struct of log translation function constants from structs above
        therm_pin: Analog pin the thermistor is wired to

        Optional arguments
        R: Resistance of resistor in series with thermistor
        vcc: Total voltage across R and thermistor
    */
    Thermistor(const Thermistor_Constants constants, int therm_pin, float R = 4700, float vcc = 3.3);

    /*
        Retrives current thermistor reading
    */
    float get_temperature();
};

#endif