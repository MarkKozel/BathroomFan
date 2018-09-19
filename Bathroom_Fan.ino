enum ControlType {
  BUTTON_TYPE,
  FAN_TYPE,
};

typedef struct {
  int pin;
  ControlType type; //const defs
  int lastState;
  int currentState;
} control;

control upBtn = {2, BUTTON_TYPE, 0, 0};
const int UP_BUTTON = 0; //Up Btn control index

control fan = {3, FAN_TYPE, 0, 0};
const int FAN = 1; //Fan control index
const int FAN_UP = 0;
const int FAN_DOWN = 1;
const int MAX_FAN_DC = 79;

control Controls[] = {upBtn, fan};

const int dp = 8;

volatile int upButtonState = 0;
const int debounceTime = 200; //ms

unsigned long startTime;
int minuteCount = 0;

int num_array[10][7] = {  { 1,1,1,1,1,1,0 },    // 0
                          { 0,1,1,0,0,0,0 },    // 1
                          { 1,1,0,1,1,0,1 },    // 2
                          { 1,1,1,1,0,0,1 },    // 3
                          { 0,1,1,0,0,1,1 },    // 4
                          { 1,0,1,1,0,1,1 },    // 5
                          { 1,0,1,1,1,1,1 },    // 6
                          { 1,1,1,0,0,0,0 },    // 7
                          { 1,1,1,1,1,1,1 },    // 8
                          { 1,1,1,0,0,1,1 }};   // 9



void setup() {
  Serial.begin(9600);
  pinMode(Controls[FAN].pin, OUTPUT);
  pwm25kHzBegin();
  pwmDuty(Controls[UP_BUTTON].currentState);

  pinMode(Controls[UP_BUTTON].pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Controls[UP_BUTTON].pin), buttonPressed, CHANGE);

  pinMode(dp,OUTPUT);

  pinMode(7, OUTPUT);   
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
}

void loop() {  

  for (int counter = 10; counter > 0; --counter) 
  {
   delay(500);
   Num_Write(counter-1); 
  }
  delay(500);
  
//    for(int i=7;i<13;i++)
//  {
//    digitalWrite(i,HIGH);
//    delay(600);
//  }
//  
//  // loop to turn leds od seven seg OFF
//  for(int i=7;i<13;i++)
//  {
//    digitalWrite(i,LOW);
//    delay(600);
//  }
//  
//  
//  delay(1000);
}

void Num_Write(int number) 
{
  int pin= 7;
  for (int j=0; j < 7; j++) {
   digitalWrite(pin, num_array[number][j]);
   pin++;
  }
}

void buttonPressed(){
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than debounceTime ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > debounceTime) 
    {
      Controls[UP_BUTTON].currentState = digitalRead(Controls[UP_BUTTON].pin);
      if(Controls[UP_BUTTON].lastState != Controls[UP_BUTTON].currentState){
        Serial.println(Controls[UP_BUTTON].currentState);
        Controls[UP_BUTTON].lastState = Controls[UP_BUTTON].currentState;
        
        if(Controls[FAN].currentState < MAX_FAN_DC) {
          Controls[FAN].currentState += 7;
          if(Controls[FAN].currentState > MAX_FAN_DC) {
            Controls[FAN].currentState = MAX_FAN_DC;
          }
          Serial.print("Fan set to: ");
          Serial.println(Controls[FAN].currentState);
          pwmDuty(Controls[FAN].currentState);
        }
    
      }
    }
  last_interrupt_time = interrupt_time;
}

//int changeFanSpeed(int direction){
//    if(Controls[FAN].currentState < MAX_FAN_DC) {
//      Controls[FAN].currentState += 7;
//      if(Controls[FAN].currentState > MAX_FAN_DC) {
//        Controls[FAN].currentState = MAX_FAN_DC;
//      }
//      Serial.print("Fan set to: ");
//      Serial.println(Controls[FAN].currentState);
//      pwmDuty(Controls[FAN].currentState);
//    }
//}

void pwm25kHzBegin() {
  TCCR2A = 0;                               // TC2 Control Register A
  TCCR2B = 0;                               // TC2 Control Register B
  TIMSK2 = 0;                               // TC2 Interrupt Mask Register
  TIFR2 = 0;                                // TC2 Interrupt Flag Register
  TCCR2A |= (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);  // OC2B cleared/set on match when up/down counting, fast PWM
  TCCR2B |= (1 << WGM22) | (1 << CS21);     // prescaler 8
  OCR2A = 79;                               // TOP overflow value (Hz)
  OCR2B = 0;
}

void pwmDuty(byte ocrb) {
  OCR2B = ocrb;                             // PWM Width (duty)
  Serial.print("Fan ordered to ");
  Serial.println(ocrb);
}
