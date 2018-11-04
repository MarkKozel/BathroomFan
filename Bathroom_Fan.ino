#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

//#define LOGO16_GLCD_HEIGHT 16 
//#define LOGO16_GLCD_WIDTH  16 
//static const unsigned char PROGMEM logo16_glcd_bmp[] =
//{ B00000000, B11000000,
//  B00000001, B11000000,
//  B00000001, B11000000,
//  B00000011, B11100000,
//  B11110011, B11100000,
//  B11111110, B11111000,
//  B01111110, B11111111,
//  B00110011, B10011111,
//  B00011111, B11111100,
//  B00001101, B01110000,
//  B00011011, B10100000,
//  B00111111, B11100000,
//  B00111111, B11110000,
//  B01111100, B11110000,
//  B01110000, B01110000,
//  B00000000, B00110000 };

char displayString[5];
boolean keepAliveOn = false;
boolean updateDisplay = false;

const int BUTTON_Pin = 2;
volatile boolean keyPressed = false;

// variable used for the debounce
unsigned long timeNewKeyPress = 0;
unsigned long timeLastKeyPress = 0;
unsigned int timeDebounce = 100;

const int LED_Pin = 7;
// variable used to control the LED
boolean LEDstatus = LOW;

//int buttonCurrentState = 0;
//int buttonLastState = 0;

const int FAN_Pin = 3; //Fan control index
int fanCurrentState = 0;
const int FAN_UP = 0;
const int FAN_DOWN = 1;
const int MAX_FAN_DC = 79;

// const int led_pin = PB5; //Digital Pin 13 and on-board led
const uint16_t t1_load = 0;
const uint16_t t1_comp = 62245; //Compare value 256 -  (1s * 16MHz)/256

int minuteCount = 0;
int countdownSeconds = 0;
const int MINUTE_COUNT_MAX = 15;


//For 7-segment LED display
#include <ShiftRegister74HC595.h>
int dataPin = 4;
int latchPin = 5;
int clockPin = 6;
// create shift register object (number of shift registers, data pin, clock pin, latch pin)
ShiftRegister74HC595 sr (1, dataPin, clockPin, latchPin);

boolean buttonWasPressed;

void setup()
{
  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay();
  display.drawPixel(10, 10, WHITE);
  display.display();

  UpdateOLEDDisplay(0);
  
  //myfnUpdateDisplay(myfnNumToBits(-1));

  //Setup fan and init PWM on
  pinMode(FAN_Pin, OUTPUT);
  pwm25kHzBegin();
  pwmDuty(0);

  pinMode(BUTTON_Pin, INPUT);
  //digitalWrite(BUTTON_Pin, LOW);
  attachInterrupt( digitalPinToInterrupt(BUTTON_Pin), keyIsPressed, RISING );

  pinMode(LED_Pin, OUTPUT);  
   digitalWrite(LED_Pin,LOW); 
   
  initTimer1();
}

void loop()
{
  if (keyPressed)
  {
      keyPressed = false;
      timeNewKeyPress = millis();
  
      if ( timeNewKeyPress - timeLastKeyPress >= timeDebounce)
      {
          Serial.print("button: ");
          Serial.println(digitalRead(BUTTON_Pin));
          addMinuteToTime();
          blink();
      }
      timeLastKeyPress = timeNewKeyPress;
  }

  if(updateDisplay){
    display.display();
    updateDisplay = false;
  }
}

void blink()
{
      if (LEDstatus == LOW) { LEDstatus = HIGH; } else { LEDstatus = LOW; }   
      digitalWrite(LED_Pin, LEDstatus);
}

//ISR for button press
//Sets flag and leaves
void keyIsPressed()
{
   keyPressed = true;
}


int checkForTimesUp()
{   
  if (countdownSeconds > 0)
  { //Still time. Count down and check for minute mark
    countdownSeconds--;

    if ((countdownSeconds % 60) == 0)
    {
      minuteCount--;
       //myfnUpdateDisplay(myfnNumToBits(minuteCount));
       UpdateOLEDDisplay(minuteCount);
    }

    Serial.print("   countdownSeconds: ");
    Serial.println(countdownSeconds);

    Serial.print("   minuteCount: ");
    Serial.println(minuteCount);

//    toggleKeepAlive();
  }
  else
  { //Zero seconds. Clean up and turn off fan
    countdownSeconds = 0;
    minuteCount = 0;

    Serial.print("   countdownSeconds: ");
    Serial.println(countdownSeconds);

    Serial.print("   minuteCount: ");
    Serial.println(minuteCount);

    //toggleKeepAlive();

     //myfnUpdateDisplay(myfnNumToBits(minuteCount));
     UpdateOLEDDisplay(minuteCount);

    if (fanCurrentState != 0)
    {
      fanCurrentState = 0;
      pwmDuty(fanCurrentState); //Turn off fan
    }
  }
}

void addMinuteToTime()
{
  Serial.println("addMinuteToTime");
  if (minuteCount < MINUTE_COUNT_MAX)
  {
    countdownSeconds = countdownSeconds + 60; //convert to seconds
    minuteCount = countdownSeconds / 60;
//    Num_Write(minuteCount);
    //myfnUpdateDisplay(myfnNumToBits(minuteCount));
    UpdateOLEDDisplay(minuteCount);

    Serial.print("countdownSeconds is now ");
    Serial.println(countdownSeconds);
    Serial.print("minuteCount is now ");
    Serial.println(minuteCount);

    fanCurrentState = MAX_FAN_DC;
    pwmDuty(fanCurrentState); //Turn on fan
  }
  else
  {
    Serial.print("minuteCount already maxed out at ");
    Serial.println(minuteCount);
    Serial.print("Max Value is  ");
    Serial.println(MINUTE_COUNT_MAX);
  }
}

void pwm25kHzBegin()
{
  TCCR2A = 0;                                            // TC2 Control Register A
  TCCR2B = 0;                                            // TC2 Control Register B
  TIMSK2 = 0;                                            // TC2 Interrupt Mask Register
  TIFR2 = 0;                                             // TC2 Interrupt Flag Register
  TCCR2A |= (1 << COM2B1) | (1 << WGM21) | (1 << WGM20); // OC2B cleared/set on match when up/down counting, fast PWM
  TCCR2B |= (1 << WGM22) | (1 << CS21);                  // prescaler 8
  OCR2A = 79;                                            // TOP overflow value (Hz)
  OCR2B = 0;
}

void pwmDuty(byte ocrb)
{
  OCR2B = ocrb; // PWM Width (duty)
  Serial.print("Fan ordered to ");
  Serial.println(ocrb);
}

void initTimer1()
{
  //Reset Time1 Ctrl Reg A to default
  TCCR1A = 0;

  //Set prescalar to 256 for 1 second timer (b100)
  TCCR1B |= (1 << CS12);
  TCCR1B &= ~(1 << CS11);
  TCCR1B &= ~(1 << CS10);

  //ResetTimer1 and set compare value
  TCNT1 = t1_load;
  OCR1A = t1_comp;

  //Set Timer1 compare interrupt enable
  TIMSK1 = (1 << OCIE1A);

  //Enable global inettupts
  sei();
}

//ISA for 1 second check/keep alive
ISR(TIMER1_COMPA_vect)
{
  checkForTimesUp();
  toggleKeepAlive();
  TCNT1 = t1_load;
}

void toggleKeepAlive() {
  uint8_t Toggle_Color;
  
  if(keepAliveOn) {
    Toggle_Color = BLACK;
    keepAliveOn = false;
  } else {
    Toggle_Color = WHITE;
    keepAliveOn = true;
  }
    display.fillRoundRect(123, 27, 5, 5, 1, Toggle_Color);
    
    updateDisplay = true;
}

void UpdateOLEDDisplay(int time) {
  sprintf(displayString, "%d min", time);
  Serial.print("displayString() => ");
  Serial.println(displayString);

  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(displayString);

  updateDisplay = true;
}

//void myfnUpdateDisplay(byte eightBits) {
//  digitalWrite(latchPin, LOW);  // prepare shift register for data
//  shiftOut(dataPin, clockPin, LSBFIRST, eightBits); // send data
//  digitalWrite(latchPin, HIGH); // update display
//}
//
//byte myfnNumToBits(int someNumber) {
//  switch (someNumber) {
//    case -1: //Clear
//      return B00000000;
//      break;
//    case 0:
//      return B11111100;
//      break;
//    case 1:
//      return B01100000;
//      break;
//    case 2:
//      return B11011010;
//      break;
//    case 3:
//      return B11110010;
//      break;
//    case 4:
//      return B01100110;
//      break;
//    case 5:
//      return B10110110;
//      break;
//    case 6:
//      return B10111110;
//      break;
//    case 7:
//      return B11100000;
//      break;
//    case 8:
//      return B11111110;
//      break;
//    case 9:
//      return B11110110;
//      break;
//    case 10:
//      return B11101110; // Hexidecimal A
//      break;
//    case 11:
//      return B00111110; // Hexidecimal B
//      break;
//    case 12:
//      return B10011100; // Hexidecimal C
//      break;
//    case 13:
//      return B01111010; // Hexidecimal D
//      break;
//    case 14:
//      return B10011110; // Hexidecimal E
//      break;
//    case 15:
//      return B10001110; // Hexidecimal F
//      break;  
//    default:
//      return B10010010; // Error condition, displays three vertical bars
//      break;   
//  }
//}
