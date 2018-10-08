
const int BUTTON_Pin = 2;
int buttonCurrentState = 0;
int buttonLastState = 0;

const int FAN_Pin = 3; //Fan control index
int fanCurrentState = 0;
const int FAN_UP = 0;
const int FAN_DOWN = 1;
const int MAX_FAN_DC = 79;

// const int led_pin = PB5; //Digital Pin 13 and on-board led
const uint16_t t1_load = 0;
const uint16_t t1_comp = 26500; //Compare value 256 -  (0.5s * 16MHz)/256

int minuteCount = 0;
int countdownSeconds = 0;
const int MINUTE_COUNT_MAX = 9;

//For 7-segment LED display
int num_array[10][7] = {{1, 1, 1, 1, 1, 1, 0},  // 0
                        {0, 1, 1, 0, 0, 0, 0},  // 1
                        {1, 1, 0, 1, 1, 0, 1},  // 2
                        {1, 1, 1, 1, 0, 0, 1},  // 3
                        {0, 1, 1, 0, 0, 1, 1},  // 4
                        {1, 0, 1, 1, 0, 1, 1},  // 5
                        {1, 0, 1, 1, 1, 1, 1},  // 6
                        {1, 1, 1, 0, 0, 0, 0},  // 7
                        {1, 1, 1, 1, 1, 1, 1},  // 8
                        {1, 1, 1, 0, 0, 1, 1}}; // 9

boolean buttonWasPressed;

void setup()
{
  Serial.begin(9600);

  //Setup fan and init PWM on
  pinMode(FAN_Pin, OUTPUT);
  pwm25kHzBegin();
  pwmDuty(0);

  pinMode(BUTTON_Pin, INPUT);
  digitalWrite(BUTTON_Pin, LOW);

  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);

  // DDRB |= (1 << led_pin);

  initTimer1();
}

void loop()
{
  buttonCurrentState = digitalRead(BUTTON_Pin);
  if (buttonCurrentState != buttonLastState)
  {
    if(buttonCurrentState = HIGH) {
//      digitalWrite(BUTTON_Pin, LOW);
      Serial.print("button: ");
      Serial.println(digitalRead(BUTTON_Pin));
      addMinuteToTime();
    }
    
  }
  buttonLastState = buttonCurrentState;
  delay(100);
}

int checkForTimesUp()
{
  if (countdownSeconds > 0)
  { //Still time. Count down and check for minute mark
    countdownSeconds--;

    if ((countdownSeconds % 60) == 0)
    {
      minuteCount--;
      Num_Write(minuteCount);
    }

    Serial.print("   countdownSeconds: ");
    Serial.println(countdownSeconds);

    Serial.print("   minuteCount: ");
    Serial.println(minuteCount);
  }
  else
  { //Zero seconds. Clean up and turn off fan
    countdownSeconds = 0;
    minuteCount = 0;

    Serial.print("   countdownSeconds: ");
    Serial.println(countdownSeconds);

    Serial.print("   minuteCount: ");
    Serial.println(minuteCount);

    Num_Write(minuteCount);

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
    Num_Write(minuteCount);

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

void Num_Write(int number)
{
  int pin = 7;
  for (int j = 0; j < 7; j++)
  {
    digitalWrite(pin, num_array[number][j]);
    pin++;
  }
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

ISR(TIMER1_COMPA_vect)
{
  checkForTimesUp();
  TCNT1 = t1_load;
}
