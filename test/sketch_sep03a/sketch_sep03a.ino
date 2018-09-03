volatile unsigned long overflowCount;
volatile unsigned long prevTimeStamp;
volatile unsigned long currentTimeStamp;
volatile unsigned int channel;
volatile unsigned long input[6];
 

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

  noInterrupts ();  // protected code

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
  interrupts ();
}

void loop() {
  for (int channel = 0; channel < 6; channel++)
  {
    Serial.print(String(input[channel]) + " ");
  }
  Serial.println();
}
