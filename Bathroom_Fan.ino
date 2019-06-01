#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <Chrono.h>

extern "C"
{
#include "filter.h";
};

#define OLED_RESET 4
Adafruit_SSD1306 OLED(OLED_RESET);

// elements for OLED Display
char displayTime[15];
char displayRh[16];
boolean keepAliveOn = false;
boolean updateDisplay = false;

// Elements for Add Minute push button
const int OneMin_BUTTON_Pin = 6;
int oneButtonState;
int oneLastState = LOW;
unsigned long oneLastDbTime = 0;
unsigned long oneDbDelay = 25;

// Elements for Add 10 Minutes push button
const int TenMin_BUTTON_Pin = 5;
int tenButtonState;
int tenLastState = LOW;
unsigned long tenLastDbTime = 0;
unsigned long tenDbDelay = 25;

const int FAN_Pin = 3; // Fan control index
int fanCurrentState = 0;
const int MAX_FAN_DC = 79;

// const int led_pin = PB5; //Digital Pin 13 and on-board led
// const uint16_t t1_load = 0;
// const uint16_t t1_comp = 62245; // Compare value 256 -  (1s * 16MHz)/256

const int MINUTE_COUNT_MAX = 30;

// Temp Sensor
const int DHTPIN = 8;
DHT_Unified dht_u(DHTPIN, DHT21);
uint32_t delayMS;

float tempAveragedValue;
float rhAveragedValue;

boolean addMinuteForRh = false;
boolean fanOnForRh = false;
const int RH_FAN_ON = 50;
const int RH_FAN_OFF = 110;

//Metro Timers
Chrono timerDisplay;
Chrono timerKeyPressed;
Chrono timerSensorReadings;
Chrono timerOneMinute(Chrono::SECONDS);

int minutesLeftOnFan = 0;

void setup()
{
  Serial.begin(115200);

  // initialize with the I2C addr 0x3C (for the 128x32)
  OLED.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  OLED.clearDisplay();
  OLED.drawPixel(10, 10, WHITE);
  OLED.display();

  UpdateOLEDDisplay(0);

  // Setup fan and init PWM on
  pinMode(FAN_Pin, OUTPUT);
  pwm25kHzBegin();
  pwmDuty(0);

  pinMode(OneMin_BUTTON_Pin, INPUT);
  pinMode(TenMin_BUTTON_Pin, INPUT);

  dht_u.begin();
  sensor_t sensor;
  dht_u.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print(F("Sensor Type: "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:  "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:   "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:   "));
  Serial.print(sensor.max_value);
  Serial.println(F("°C"));
  Serial.print(F("Min Value:   "));
  Serial.print(sensor.min_value);
  Serial.println(F("°C"));
  Serial.print(F("Resolution:  "));
  Serial.print(sensor.resolution);
  Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht_u.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print(F("Sensor Type: "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:  "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:   "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:   "));
  Serial.print(sensor.max_value);
  Serial.println(F("%"));
  Serial.print(F("Min Value:   "));
  Serial.print(sensor.min_value);
  Serial.println(F("%"));
  Serial.print(F("Resolution:  "));
  Serial.print(sensor.resolution);
  Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
}

void loop()
{
  //Metro timer for Display Updates
  if (timerDisplay.hasPassed(250))
  {
    timerDisplay.restart();
    if (updateDisplay)
    {
      OLED.display();
      updateDisplay = false;
    }
  }

  //Metro timer for Sensor Updates
  if (timerSensorReadings.hasPassed(500))
  {
    timerSensorReadings.restart();
    newTempReading();
    toggleKeepAlive();
  }

  //Check for end of fan run time (minutesLeftOnFan). Decrement and
  //turn off fan if time has ended
  if (timerOneMinute.hasPassed(60))
  {
    timerOneMinute.restart();
    if (minutesLeftOnFan > 0)
    {
      minutesLeftOnFan--;

      if ((minutesLeftOnFan == 0) && (fanCurrentState != 0))
      { //Time's up, turn off fan
        fanCurrentState = 0;
        pwmDuty(fanCurrentState); // Turn off fan
      }
      UpdateOLEDDisplay(minutesLeftOnFan);
    }
  }

  if (timerKeyPressed.hasPassed(100))
  {
    timerKeyPressed.restart();

    //1 minute button
    int oneRead = digitalRead(OneMin_BUTTON_Pin);
    if (oneRead != oneLastState)
    {
      oneLastDbTime = millis();
    }

    if ((millis() - oneLastDbTime) > oneDbDelay)
    {
      if (oneRead != oneButtonState)
      {
        oneButtonState = oneRead;
        if (oneButtonState == HIGH)
        {
          addToFanTime(1);
        }
      }
    }
    oneLastState = oneRead;

    //10 minute button
    int tenRead = digitalRead(TenMin_BUTTON_Pin);
    if (tenRead != tenLastState)
    {
      tenLastDbTime = millis();
    }

    if ((millis() - tenLastDbTime) > tenDbDelay)
    {
      if (tenRead != tenButtonState)
      {
        tenButtonState = tenRead;
        if (tenButtonState == HIGH)
        {
          addToFanTime(10);
        }
      }
    }
    tenLastState = tenRead;
  }
}

// Called by 1-second ISR to sample gas. If 10 points of data (gasReadingCount)
// calculate current average as check to see if fan should turn on
void newTempReading()
{
  float newRh;
  float newTemp;

  char temp_str[5];
  char rh_str[5];

  delay(delayMS);

  sensors_event_t eventT;
  sensors_event_t eventH;

  dht_u.temperature().getEvent(&eventT);
  // newTemp = (float)eventT.temperature * 1.8 + 32;
   newTemp = (float)eventT.temperature;

  dht_u.humidity().getEvent(&eventH);
  newRh = (float)eventH.relative_humidity;

  if (isnan(newRh) || isnan(newTemp))
  {
    Serial.println(F("Error reading temperature/humidity!"));
  }
  else
  {
    insertData(newRh, newTemp);
  }

  rhAveragedValue = getRh();
  tempAveragedValue = getTemp();

  Serial.print("RH (Imm/Ave)= ");
  Serial.print(newRh);
  Serial.print("/");
  Serial.println(rhAveragedValue);

  Serial.print("Temp (Imm/Ave) = ");
  Serial.print(newTemp);
  Serial.print("/");
  Serial.println(tempAveragedValue);

  if (!fanOnForRh && (minutesLeftOnFan == 0) && (newRh >= RH_FAN_ON))
  {
    addToFanTime(1);
    fanOnForRh = true;
    Serial.println("Fan on for Rh");
  }
  else
  {
    fanOnForRh = false;
  }

  dtostrf(tempAveragedValue, 3, 0, temp_str);
  dtostrf(rhAveragedValue, 3, 0, rh_str);
  sprintf(displayRh, "%sF %s%%", temp_str, rh_str);
  Serial.println(displayRh);
  OLED.setTextSize(2);

  OLED.setCursor(0, 16);
  OLED.fillRect(0, 16, 128, 16, BLACK);
  OLED.setTextColor(WHITE);
  OLED.println(displayRh);
  updateDisplay = true;
}

// Called when button is pressed. Adds 1 minute unless already at
// MINUTE_COUNT_MAX
void addToFanTime(int timeToAdd)
{
  if (minutesLeftOnFan < MINUTE_COUNT_MAX)
  {
    // countdownSeconds = countdownSeconds + 60; // convert to seconds
    minutesLeftOnFan += timeToAdd;
    if (minutesLeftOnFan > MINUTE_COUNT_MAX)
    {
      minutesLeftOnFan = MINUTE_COUNT_MAX;
    }
    fanCurrentState = MAX_FAN_DC;
    pwmDuty(fanCurrentState); // Turn on fan
    UpdateOLEDDisplay(minutesLeftOnFan);
  }
}

// Setup TC2 for PWM Fan Control
void pwm25kHzBegin()
{
  TCCR2A = 0; // TC2 Control Register A
  TCCR2B = 0; // TC2 Control Register B
  TIMSK2 = 0; // TC2 Interrupt Mask Register
  TIFR2 = 0;  // TC2 Interrupt Flag Register
  TCCR2A |=
      (1 << COM2B1) | (1 << WGM21) |
      (1 << WGM20);                     // OC2B cleared/set on match when up/down counting, fast PWM
  TCCR2B |= (1 << WGM22) | (1 << CS21); // prescaler 8
  OCR2A = 79;                           // TOP overflow value (Hz)
  OCR2B = 0;
}

// Set/Change PWM Fan duty cycle
void pwmDuty(byte ocrb)
{
  OCR2B = ocrb; // PWM Width (duty)
}

// Update Keep Alive graphic. Wait for 1-second interrupt to update display
void toggleKeepAlive()
{
  uint8_t Toggle_Color;

  if (keepAliveOn)
  {
    Toggle_Color = BLACK;
    keepAliveOn = false;
  }
  else
  {
    Toggle_Color = WHITE;
    keepAliveOn = true;
  }
  OLED.fillRoundRect(123, 5, 5, 5, 1, Toggle_Color);

  updateDisplay = true;
}

// Update timer on diplay. Wait for 1-second interrupt to update display
void UpdateOLEDDisplay(int time)
{
  sprintf(displayTime, "Time: %d", time);

  OLED.clearDisplay();
  OLED.setTextSize(2);
  OLED.setTextColor(WHITE);

  OLED.setCursor(0, 0);
  OLED.println(displayTime);

  updateDisplay = true;
}
