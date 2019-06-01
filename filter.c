#include "filter.h"
#include <stdio.h>

#define DATA_DEPTH 10
#define RH_INDEX 0
#define TEMP_INDEX 1

float readings[2][10] = {0};
const int MAX_READINGS = 10;
int currentIndex = 0;
int currentCount = 0;

/*
* Filter to smooth Gas readings
* Manage gasReadings int array, set to size MAX_READINGS
* insets newGasReading into oldest position in array
* currentGasIndex points to oldest position
* currentGasCount is number of data elements in array
* Returns average is data elements
*/
void insertData(float newRh, float newTemp)
{
  readings[RH_INDEX][currentIndex] = (float)newRh;
  readings[TEMP_INDEX][currentIndex] = (float)newTemp;
  currentCount++;
  currentIndex++;

  if (currentCount >= MAX_READINGS)
  {
    currentCount = MAX_READINGS; //Cannot grow bigger than MAX_READINGS
  }

  //Reset currentGasIndex is >= MAX_READINGS
  if (currentIndex >= MAX_READINGS)
  {
    currentIndex = 0; //Reset when >= MAX_READINGS
  }

  // Serial.print("currentCount => ");
  // Serial.print(currentCount);
  // Serial.print("    currentIndex => ");
  // Serial.println(currentIndex);

  //  Serial.print("RH => ");
  // for (int x = 0; x < currentCount; x++)
  // {
  //   Serial.print(readings[RH_INDEX][x]);
  //   Serial.print(", ");
  // }
  // Serial.println("");

  // Serial.print("TP => ");
  // for (int x = 0; x < currentCount; x++)
  // {
  //   Serial.print(readings[TEMP_INDEX][x]);
  //   Serial.print(", ");
  // }
  // Serial.println("");
}

float getTemp()
{
  float result = 0.0;
  //Calculate filter average if 10 readings
  for (int x = 0; x < currentCount; x++)
  {
    result += (float)readings[TEMP_INDEX][x];
  }
  return (float)(result / (float)currentCount);
  // return result;
}

float getRh()
{
  float result = 0.0;
  //Calculate filter average if 10 readings
  for (int x = 0; x < currentCount; x++)
  {
    result += (float)readings[RH_INDEX][x];
    // Serial.print("RH Average Loop: ");
    // Serial.print((float)readings[RH_INDEX][x]);
    //  Serial.print(" added to total ");
    //   Serial.println(result);
  }
  // Serial.print("RH Average ttl=");
  // Serial.print(result);
  // Serial.print("  count=");
  // Serial.print((float)currentCount);
  // Serial.print("   = ");
  // Serial.println((float)(result / (float)currentCount));
  return (float)(result / (float)currentCount);
  // return result;
}