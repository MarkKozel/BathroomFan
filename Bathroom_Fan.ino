#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>

// Uncomment to see debugging info in Serial Console
//#define DEBUG 1

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// elements for OLED Display
char displayString[5];
boolean keepAliveOn = false;
boolean updateDisplay = false;

// Elements for Add Minute push button
const int BUTTON_Pin = 2;
volatile boolean keyPressed = false;
unsigned long timeNewKeyPress = 0;
unsigned long timeLastKeyPress = 0;
unsigned int timeDebounce = 250;

const int FAN_Pin = 3; // Fan control index
int fanCurrentState = 0;
const int FAN_UP = 0;
const int FAN_DOWN = 1;
const int MAX_FAN_DC = 79;

// const int led_pin = PB5; //Digital Pin 13 and on-board led
const uint16_t t1_load = 0;
const uint16_t t1_comp = 62245; // Compare value 256 -  (1s * 16MHz)/256

int minuteCount = 0;
int countdownSeconds = 0;
const int MINUTE_COUNT_MAX = 30;

void setup()
{
  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC,
                0x3C); // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay();
  display.drawPixel(10, 10, WHITE);
  display.display();

  UpdateOLEDDisplay(0);

  // Setup fan and init PWM on
  pinMode(FAN_Pin, OUTPUT);
  pwm25kHzBegin();
  pwmDuty(0);

  pinMode(BUTTON_Pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_Pin), keyIsPressed, FALLING);

  initTimer1();
}

void loop()
{
  if (keyPressed)
  {
    keyPressed = false;
    timeNewKeyPress = millis();

    if (timeNewKeyPress - timeLastKeyPress >= timeDebounce)
    {
#ifdef DEBUG
      Serial.print("button: ");
      Serial.println(digitalRead(BUTTON_Pin));
#endif
      addMinuteToTime();
    }
    timeLastKeyPress = timeNewKeyPress;
  }

  if (updateDisplay)
  {
    display.display();
    updateDisplay = false;
  }
}

// ISR for button press. Sets flag and leaves
void keyIsPressed() { keyPressed = true; }

// Checks for end of timer to turn off fan.
// Decrements second and minute (if needed) counters
// Updates display if minute counter changed
// Called from 1-second ISR
int checkForTimesUp()
{
  if (countdownSeconds > 0)
  { // Still time. Count down and check for minute mark
    countdownSeconds--;

    if ((countdownSeconds % 60) == 0)
    {
      minuteCount--;
      UpdateOLEDDisplay(minuteCount);
    }
#ifdef DEBUG
    Serial.print("   countdownSeconds: ");
    Serial.println(countdownSeconds);

    Serial.print("   minuteCount: ");
    Serial.println(minuteCount);
#endif
  }
  else
  { // Zero seconds. Clean up and turn off fan
    countdownSeconds = 0;
    minuteCount = 0;
#ifdef DEBUG
    Serial.print("   countdownSeconds: ");
    Serial.println(countdownSeconds);

    Serial.print("   minuteCount: ");
    Serial.println(minuteCount);
#endif

    UpdateOLEDDisplay(minuteCount);

    if (fanCurrentState != 0)
    {
      fanCurrentState = 0;
      pwmDuty(fanCurrentState); // Turn off fan
    }
  }
}

// Called when button is pressed. Adds 1 minute unless already at
// MINUTE_COUNT_MAX
void addMinuteToTime()
{
#ifdef DEBUG
  Serial.println("addMinuteToTime");
#endif
  if (minuteCount < MINUTE_COUNT_MAX)
  {
    countdownSeconds = countdownSeconds + 60; // convert to seconds
    minuteCount++;
    UpdateOLEDDisplay(minuteCount);
#ifdef DEBUG
    Serial.print("countdownSeconds is now ");
    Serial.println(countdownSeconds);
    Serial.print("minuteCount is now ");
    Serial.println(minuteCount);
#endif
    fanCurrentState = MAX_FAN_DC;
    pwmDuty(fanCurrentState); // Turn on fan
  }
#ifdef DEBUG
  else
  {
    Serial.print("minuteCount already maxed out at ");
    Serial.println(minuteCount);
    Serial.print("Max Value is  ");
    Serial.println(MINUTE_COUNT_MAX);
  }
#endif
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
#ifdef DEBUG
  Serial.print("Fan ordered to ");
  Serial.println(ocrb);
#endif
}

// Setup timer for 1-second interrupt
void initTimer1()
{
  // Reset Time1 Ctrl Reg A to default
  TCCR1A = 0;

  // Set prescalar to 256 for 1 second timer (b100)
  TCCR1B |= (1 << CS12);
  TCCR1B &= ~(1 << CS11);
  TCCR1B &= ~(1 << CS10);

  // ResetTimer1 and set compare value
  TCNT1 = t1_load;
  OCR1A = t1_comp;

  // Set Timer1 compare interrupt enable
  TIMSK1 = (1 << OCIE1A);

  // Enable global inettupts
  sei();
}

// ISA for 1-second check
ISR(TIMER1_COMPA_vect)
{
  checkForTimesUp();
  toggleKeepAlive();
  TCNT1 = t1_load;
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
  display.fillRoundRect(123, 27, 5, 5, 1, Toggle_Color);

  updateDisplay = true;
}

// Update timer on diplay. Wait for 1-second interrupt to update display
void UpdateOLEDDisplay(int time)
{
  sprintf(displayString, "%d min", time);
#ifdef DEBUG
  Serial.print("displayString() => ");
  Serial.println(displayString);
#endif
  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println(displayString);

  updateDisplay = true;
}
