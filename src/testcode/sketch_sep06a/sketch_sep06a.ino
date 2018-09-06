// определение времени выполнения программного блока Ардуино

unsigned int  timerValue; // значение таймера
  
void setup() {
  Serial.begin(9600);  // инициализируем последовательный порт, скорость 9600
  // установки таймера 1
  TCCR1A = 0;
  TCCR1B = 0;  
}

///////////////////////
#define default_servo_value (1500 * 2)  //set the default servo value
#define min_servo_value (950 * 2)       //set the min servo value
#define max_servo_value (2050 * 2)      //set the max servo value

#define PPM_FrGap (3000 * 2)            //set the gap between PPM frames microseconds (1ms = 1000Вµs)
#define PPM_FrLen (20500 * 2)           //set the PPM frame length in microseconds (1ms = 1000Вµs)
#define PPM_PulseLen (300 * 2)          //set the pulse length
#define onState 0                                        //set polarity of the pulses: 1 is positive, 0 is negative
#define OCR2A_full 249                                   //Max value for OCR2A register
#define outPin 12
  unsigned int interruptCount = 0;
  unsigned int OCR2A_rest = 0;
  unsigned int OCR2A_next = OCR2A_full;
  boolean state = true;
  boolean wait = false;
  byte channel = 0;
  unsigned long calc_rest;
const byte channelAmount = 6;                            //set number of channels, 8 channels max

//Chanels order recieving AETR1234
//Chanels order transmitting TAER1234
const unsigned int chanelsOutput[] = {2, 0, 1, 3, 4, 5, 6, 7};

volatile unsigned long overflowCount;

volatile unsigned long input[channelAmount];             //channels readed
volatile unsigned long output[channelAmount];            //channels to translate
///////////////////////

  
void loop() {
  noInterrupts(); // запрет прерываний
  TCNT1H = 0; // сброс таймера
  TCNT1L = 0;
  TCCR1B = 1; // разрешение работы таймера

  // ---------- исследуемый программный блок ---------

if (interruptCount > 0)                         // decrement interruptCount every interrupt
  {
    interruptCount--;
  }

  if (state)                                                // start pulse
  {
    if (interruptCount == 0)                                // if interruptCount == 0, then it's start of new pulse or it's last iteration in that pulse
    {
      if (!wait)                                            // start of new pulse
      {
        digitalWrite(outPin, onState);                      // write pulse polarity
        /*if(onState)
          OUT_PORT |= outPinBitMask;
        else
          OUT_PORT &= ~outPinBitMask;
        */
        interruptCount = PPM_PulseLen / (OCR2A_full + 1);   // count interrupts
        OCR2A_rest = PPM_PulseLen % (OCR2A_full + 1);
        if (interruptCount > 0)                             // pulse time takes more than one interrupt
        {
          OCR2A_next = OCR2A_full;                          // next interrupt after 250 ticks
          wait = true;
        }
        else                                                // pulse time less than one interrupt
        {
          OCR2A_next = PPM_PulseLen;
          state = false;
        }
      }
      else                                                  // last interrupt in this pulse. Change state after this
      {
        OCR2A_next = OCR2A_rest;
        state = false;
        wait = false;
      }
    }
    else                                                    // in the middle wait and interrupt every 250 ticks
    {
      OCR2A_next = OCR2A_full;
    }
  }
  else                                                      // state changed. wait for next channel pulse
  {
    if (channel >= channelAmount)                           // all channels sended. calculate syncro gap
    {
      if (interruptCount == 0)
      {
        if (!wait)
        {
          digitalWrite(outPin, !onState);
          /*if(!onState)
            OUT_PORT |= outPinBitMask;
          else
            OUT_PORT &= ~outPinBitMask;
          */
          unsigned int channelLength = (PPM_FrLen - calc_rest - PPM_PulseLen);
          interruptCount  = channelLength / (OCR2A_full + 1);
          OCR2A_rest = channelLength % (OCR2A_full + 1);
                    
          if (interruptCount > 0)                             // pulse time takes more than one interrupt
          {
            OCR2A_next = OCR2A_full;                          // next interrupt after 250 ticks
            wait = true;
          }
          else                                                // pulse time less than one interrupt
          {
            OCR2A_next = PPM_PulseLen;
            state = true;
          }
        }
        else                                                  // last interrupt in frame
        {
          OCR2A_next = OCR2A_rest;
          state = true;
          wait = false;                                       // force resync
          calc_rest = 0;
          channel = 0;
        }
      }
      else
      {
        OCR2A_next = OCR2A_full;
      }
    }
    else
    {
      if (interruptCount == 0)
      {
        if (!wait)
        {
          digitalWrite(outPin, !onState);
          unsigned long value = output[channel];
          unsigned int channelLength = value - PPM_PulseLen;  
          calc_rest += value;        
          interruptCount = channelLength / (OCR2A_full + 1);
          OCR2A_rest = channelLength % (OCR2A_full + 1);
          
          if (interruptCount > 0)                             // pulse time takes more than one interrupt
          {
            OCR2A_next = OCR2A_full;                          // next interrupt after 250 ticks
            wait = true;
          }
          else                                                // pulse time less than one interrupt
          {
            OCR2A_next = channelLength;
            state = true;
          }
        }
        else                                                  // last interrupt in channel
        {
          OCR2A_next = OCR2A_rest;          
          channel++;
          state = true;
          wait = false;
        }
      }
      else
      {
        OCR2A_next = OCR2A_full;        
      }
    }
  }

  // -------------------------------------------------

  TCCR1B = 0; // остановка таймера
  timerValue = (unsigned int)TCNT1L | ((unsigned int)TCNT1H << 8); // чтение таймера
  interrupts(); // разрешение прерываний
      
  // вывод на компьютер
  Serial.print( (float)(timerValue - 2) * 0.0625);
  Serial.println(" mks");
  delay(500);
}
