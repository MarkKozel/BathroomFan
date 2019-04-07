#include "gas.h"
#include <stdio.h>

#define MAX_GAS_READINGS 10

int gasReadings[MAX_GAS_READINGS] = {0};
int currentGasIndex = 0;
int currentGasCount = 0;

/*
* Filter to smooth Gas readings
* Manage gasReadings int array, set to size MAX_GAS_READINGS
* insets newGasReading into oldest position in array
* currentGasIndex points to oldest position
* currentGasCount is number of data elements in array
* Returns average is data elements
*/
int gasFilter(int newGasReading)
{
    int result = 0;
    gasReadings[currentGasIndex] = newGasReading;

    if (currentGasCount < MAX_GAS_READINGS)
    {
        currentGasCount++; //Cannot grow bigger than MAX_GAS_READINGS
    }

    //Reset currentGasIndex is >= MAX_GAS_READINGS
    if (currentGasIndex >= MAX_GAS_READINGS)
    {
        currentGasIndex = 0; //Reset when >= MAX_GAS_READINGS
    }
    else
    {
        currentGasIndex++; //Bump when < MAX_GAS_READINGS
    }

    //Calculate filter average if 10 readings
    for (int x = 0; x < currentGasCount; x++)
    {
        result += gasReadings[x];
        //Serial.print(gasReadings[x]);
        // Serial.print(" ");
    }
    result = (int)(result / currentGasCount);
    //   Serial.print("Average for ");
    //   Serial.print(currentGasCount);
    //   Serial.print(" data points is ");
    //   Serial.println(result);
    return result;
}