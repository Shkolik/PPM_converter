#define default_servo_value 1500  //set the default servo value
#define min_servo_value 950  //set the min servo value
#define max_servo_value 2050  //set the max servo value

#define PPM_FrGap 3000  //set the gap between PPM frames microseconds (1ms = 1000µs)
#define PPM_FrLen 20500  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_PulseLen 300  //set the pulse length
#define onState 0  //set polarity of the pulses: 1 is positive, 0 is negative
#define OCR2A_full 249;
#define outPin 1 //set PPM signal output pin

const byte channelAmount = 6; //set number of channels, 8 channels max
//Chanels order recieving AETR1234
//Chanels order transmitting TAER1234
const unsigned int chanelsOutput[] = {2, 0, 1, 3, 4, 5, 6, 7};

volatile unsigned long overflowCount;
volatile unsigned long prevTimeStamp;
volatile unsigned long currentTimeStamp;
volatile unsigned int channel;

volatile unsigned long input[channelAmount];
volatile unsigned long output[channelAmount];

volatile unsigned int interruptCount;
volatile unsigned int OCR2A_rest;


ISR(TIMER2_COMPA_vect)
{
  static boolean state = true;
  static byte channel;
  static unsigned long calc_rest;
  TCNT2 = 0; //reset counter

  if (interruptCount > 0)
  {
    interruptCount--;
  }
  else
  {
    //start pulse
    if (state) {
      digitalWrite(outPin, onState);
      interruptCount = 1;
      OCR2A = PPM_PulseLen - 250;
      state = false;
    }
    else {
      //end pulse and calculate when to start the next pulse
      digitalWrite(outPin, !onState);
      state = true;

      if (channel >= channelAmount) 
      {
        unsigned int channelLength = (PPM_FrLen - calc_rest - PPM_PulseLen);
        interruptCount = channelLength / 250;
        if(interruptCount > 1)
        {
          OCR2A = OCR2A_full;
          OCR2A_rest = channelLength % 250;
        }
        else
        {
          }
        calc_rest = 0;
        channel = 0;
      }
      else {
        unsigned long value = output[channel];
        unsigned int channelLength = value - PPM_PulseLen;
        interruptCount = channelLength / 250;
        OCR2A_rest = channelLength % 250;
        calc_rest += value;
        channel++;
      }
    }
  }
}


ISR (TIMER1_OVF_vect)
{
  overflowCount++;
}

ISR (TIMER1_CAPT_vect)
{
  unsigned int timer1CounterValue;
  timer1CounterValue = ICR1;  // see datasheet, page 117 (accessing 16-bit registers)
  unsigned long overflowCopy = overflowCount;


  // if just missed an overflow
  // 0x7FFF is magic - have to think about this.
  if ((TIFR1 & bit (TOV1)) && timer1CounterValue < 0x7FFF)
  {
    overflowCopy++;
  }

  currentTimeStamp = (overflowCopy << 16) + timer1CounterValue;

  unsigned long value = currentTimeStamp - prevTimeStamp;
  if (channel >= 6 || value >= 6000)
  {
    channel = 0;
  }
  else
  {
    input[channel] = value;
    channel++;
  }

  prevTimeStamp = currentTimeStamp;
}

void setup() {
  Serial.begin(115200);

  pinMode(outPin, OUTPUT);
  digitalWrite(outPin, !onState);  //set the PPM signal pin to the default state (off)

  noInterrupts ();  // protected code
  //disable Timer0
  TCCR0A = 0;
  TCCR0B = 0;
  // reset Timer 1
  TCCR1A = 0;
  TCCR1B = 0;

  TIFR1 = bit (ICF1) | bit (TOV1);  // clear flags so we don't get a bogus interrupt
  TCNT1 = 0;          // Counter to zero
  overflowCount = 0;  // Therefore no overflows yet

  // Timer 1 - counts clock pulses
  TIMSK1 = bit (TOIE1) | bit (ICIE1);   // interrupt on Timer 1 overflow and input capture
  // start Timer 1, 8 prescaler
  TCCR1B =  bit (CS11) | bit (ICES1);  // plus Input Capture Edge Select (rising on D8)

  //setup timer2
  //Setup Timer2 to fire every 0.5us
  TCCR2 = 0; // set entire TCCR1 register to 0
  TCNT2 = 0;

  TCCR2A = bit (WGM21); // CTC
  TCCR2B = bit(CS21);  // 8 prescaler: 0,5 microseconds at 16mhz, 1 microsecond at 8mhz (Atmega16L)
  OCR2A = 249;         // count to 250 (zero-relative)

  TIMSK2 |= bit (OCIE2A);
  interrupts ();
}

void loop() {
  for (int channel = 0; channel < 6; channel++)
  {
    Serial.print(String(input[channel]) + " ");
  }
  Serial.println();
}
