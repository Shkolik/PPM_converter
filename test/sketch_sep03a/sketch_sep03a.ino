#define default_servo_value 3000//1500  //set the default servo value
#define min_servo_value 2000//950  //set the min servo value
#define max_servo_value 4000//2050  //set the max servo value

#define PPM_FrGap 6000//3000  //set the gap between PPM frames microseconds (1ms = 1000µs)
#define PPM_FrLen 41000//20500  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_PulseLen 600//300  //set the pulse length
#define onState 0  //set polarity of the pulses: 1 is positive, 0 is negative
#define OCR2A_full 249;
#define outPin 5 //set PPM signal output pin

const bool debug = false;

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

ISR(TIMER2_COMPA_vect)
{
  static unsigned int interruptCount = 0;
  static unsigned int OCR2A_rest = 0;
  static unsigned int OCR2A_next = OCR2A_full;
  static boolean state = true;
  static boolean wait = false;
  static byte channel = 0;
  static unsigned long calc_rest;


  TCNT2 = 0; //reset counter

  if (interruptCount > 0)
  {
    interruptCount--;
  }

  //start pulse
  if (state)
  {
    if (interruptCount < 1)
    {
      if (!wait)
      {
        digitalWrite(outPin, onState);
        interruptCount = 1 + PPM_PulseLen / 250;
        wait = true;
      }
      else
      {
        OCR2A_next = OCR2A_rest;
        state = false;
        wait = false;
      }
    }
    else
    {
      OCR2A_next = OCR2A_full;
      OCR2A_rest = PPM_PulseLen % 250;
    }
  }
  else
  {
    //Serial.println(String(channel));
    //end pulse and calculate when to start the next pulse
    if (channel >= channelAmount)
    {
      unsigned int channelLength = (PPM_FrLen - calc_rest - PPM_PulseLen);
      if (interruptCount < 1)
      {
        if (!wait)
        {
          interruptCount  = channelLength / 250;
          digitalWrite(outPin, !onState);
          wait = true;
        }
        else
        {
          OCR2A_next = OCR2A_rest;
          state = true;
          wait = false;
          calc_rest = 0;
          channel = 0;
        }
      }
      else
      {
        OCR2A_next = OCR2A_full;
        OCR2A_rest = channelLength % 250;
      }

    }
    else
    {
      unsigned long value = output[channel];
      unsigned int channelLength = value - PPM_PulseLen;
      if (interruptCount < 1)
      {
        if (!wait)
        {
          interruptCount = channelLength / 250;
          digitalWrite(outPin, !onState);
          wait = true;
        }
        else
        {
          OCR2A_next = OCR2A_rest;
          calc_rest += value;
          channel++;
          state = true;
          wait = false;
        }
      }
      else
      {
        OCR2A_next = OCR2A_full;
        OCR2A_rest = channelLength % 250;
      }
    }
  }

  OCR2A = OCR2A_next;
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
  if (channel >= channelAmount || value >= PPM_FrGap)
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
  pinMode(10, OUTPUT);
  digitalWrite(outPin, !onState);  //set the PPM signal pin to the default state (off)
  digitalWrite(10, 0);
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
  TCCR2A = 0; // set entire TCCR1 register to 0
  TCCR2B = 0;
  TCNT2 = 0;

  TCCR2A = bit (WGM21); // CTC
  TCCR2B = bit(CS21);  // 8 prescaler: 0,5 microseconds at 16mhz, 1 microsecond at 8mhz (Atmega16L)
  OCR2A = OCR2A_full;         // count to 250 (zero-relative)

  TIMSK2 |= bit (OCIE2A);
  interrupts ();
}

void loop() {
  // Get latest valid values from all channels
  for (byte channel = 0; channel < channelAmount; channel++) {
    unsigned long value = input[chanelsOutput[channel]];

    output[channel] = constrain(value, min_servo_value, max_servo_value);

    if (debug)
    {
      Serial.print(String(value) + " ");
    }
  }
  if (debug)
  {
    Serial.println();
  }
}
