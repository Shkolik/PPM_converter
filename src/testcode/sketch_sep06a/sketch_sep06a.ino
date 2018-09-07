// определение времени выполнения программного блока Ардуино

unsigned int  timerValue; // значение таймера
  
void setup() {
  Serial.begin(115200);  // инициализируем последовательный порт, скорость 9600
  // установки таймера 1
  TCCR1A = 0;
  TCCR1B = 0;  
}

///////////////////////
#define default_servo_value (1500 * 2)  //set the default servo value
#define min_servo_value (950 * 2)       //set the min servo value
#define max_servo_value (2050 * 2)      //set the max servo value

#define PPM_FrGap (3000 * 2)            //set the gap between PPM frames microseconds (1ms = 1000Вµs)
#define PPM_FrLen (20610 * 2)           //set the PPM frame length in microseconds (1ms = 1000Вµs)
#define PPM_PulseLen (300 * 2)          //set the pulse length
#define onState 0                                        //set polarity of the pulses: 1 is positive, 0 is negative
#define OCR2A_full 249                                   //Max value for OCR2A register
#define outPin 12

#define outPinBitMask digitalPinToBitMask(outPin)
#define OUT_PORT PORTB


  unsigned int pulseLength = 0;
  unsigned int OCR2A_rest = 0;
  unsigned int OCR2A_next = OCR2A_full;
  boolean state = true;
  byte channel = 0;
  unsigned int frameRest = 0;
const byte channelAmount = 6;                            //set number of channels, 8 channels max

//Chanels order recieving AETR1234
//Chanels order transmitting TAER1234
const unsigned int chanelsOutput[] = {2, 0, 1, 3, 4, 5, 6, 7};

volatile unsigned long overflowCount;

volatile unsigned long input[channelAmount];             //channels readed
volatile unsigned int output[channelAmount] = {default_servo_value, default_servo_value,default_servo_value,default_servo_value,default_servo_value,default_servo_value};            //channels to translate
///////////////////////


  unsigned int branch = 0;
  unsigned int branch1 = 0;
void loop() {
  noInterrupts(); // запрет прерываний
  TCNT1H = 0; // сброс таймера
  TCNT1L = 0;
  TCCR1B = 1; // разрешение работы таймера

  // ---------- исследуемый программный блок ---------


  if (state)                                                // start pulse
  {
    if (pulseLength == 0)                                // if interruptCount == 0, then it's start of new pulse or it's last iteration in that pulse
    {      
        // write pulse polarity
        if(onState)
          OUT_PORT |= outPinBitMask;
        else
          OUT_PORT &= ~outPinBitMask;
        
        pulseLength = PPM_PulseLen;   // count interrupts
        
          branch = 1;
          OCR2A_next = OCR2A_full;                          // next interrupt after 250 ticks
    }
    else if( pulseLength > 250)                                                 // in the middle wait and interrupt every 250 ticks
    {
      branch = 4;
      OCR2A_next = OCR2A_full;
    }
    else                                                  // last interrupt in this pulse. Change state after this
    {
        branch = 3;
        OCR2A_next = pulseLength;
        state = false;         
    }
  }
  else                                                      // state changed. wait for next channel pulse
  {    
    if (channel >= channelAmount)                           // all channels sended. calculate syncro gap
    {
      if (pulseLength == 0)
      {
        
          if(!onState)
            OUT_PORT |= outPinBitMask;
          else
            OUT_PORT &= ~outPinBitMask;
          
            pulseLength = frameRest;
          
            branch = 5;
            OCR2A_next = OCR2A_full;                          // next interrupt after 250 ticks
      }
      else if(pulseLength > 250)
      {
        branch = 8;
        OCR2A_next = OCR2A_full;
      }
      else                                                  // last interrupt in frame
        {
          branch = 7;
          OCR2A_next = pulseLength;
          state = true;
          channel = 0;
        }
    }
    else
    {
      if (pulseLength == 0)
      {
          if(!onState)
            OUT_PORT |= outPinBitMask;
          else
            OUT_PORT &= ~outPinBitMask;
          
          pulseLength = output[channel];// - PPM_PulseLen;  
          
          branch = 9;
          OCR2A_next = OCR2A_full;                          // next interrupt after 250 ticks
      }
      else if(pulseLength > 250)
      {
        branch = 12;
        OCR2A_next = OCR2A_full;        
      }
      else                                                  // last interrupt in channel
      {
        branch = 11;
        OCR2A_next = pulseLength;          
        channel++;
        state = true;
      }
    }
  }

  if (pulseLength > 250)                         // decrement interruptCount every interrupt
  {
    pulseLength -= 250;
  }
  else
  {
    pulseLength = 0;
  }

  if (frameRest > 250)                         // decrement interruptCount every interrupt
  {
    frameRest -= 250;
  }
  else
  {
    frameRest = PPM_FrLen;
  }
  
  // -------------------------------------------------

  TCCR1B = 0; // остановка таймера
  timerValue = (unsigned int)TCNT1L | ((unsigned int)TCNT1H << 8); // чтение таймера
  interrupts(); // разрешение прерываний

      if(branch == 91 || branch == 55 || branch == 71)
      {
  // вывод на компьютер
  Serial.println();
  Serial.println("--- BRANCH " + String(branch) + " ---");
  Serial.println("channel: " + String(channel));
  Serial.println("frameRest: " + String(frameRest));
  Serial.print( (float)(timerValue - 2) * 0.0625);
  Serial.println(" mks");
  Serial.println();
  delay(500);
      }
}
