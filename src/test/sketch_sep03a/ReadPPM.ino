#ifdef __AVR_ATmega16__
  #define IsLostInterrupt (TIFR & bit (TOV1))
#else   
  #define IsLostInterrupt (TIFR1 & bit (TOV1))
#endif

void setupReader()
{
  overflowCount = 0;                      // Therefore no overflows yet

  TCCR1A = 0;                             // set entire TCCR1 register to 0
  TCCR1B = 0;  
  TCCR1B = bit (CS11) | bit (ICES1);      // 8 prescaler plus Input Capture Edge Select (rising on D8)
  TCNT1 = 0;                              // Counter to zero
  
  #ifdef __AVR_ATmega16__
    //TIFR = bit (ICF1) | bit (TOV1);       // clear flags so we don't get a bogus interrupt
    TIMSK |= bit (TOIE1) | bit (TICIE1);   // interrupt on Timer 1 overflow and input capture
  #else    
    TIFR1 = bit (ICF1) | bit (TOV1);      // clear flags so we don't get a bogus interrupt    
    TIMSK1 = bit (TOIE1) | bit (ICIE1);   // interrupt on Timer 1 overflow and input capture
  #endif
}

//overflow interrupt track overflowCount
ISR (TIMER1_OVF_vect)
{
  overflowCount++;
}

//Input capture interrupt
ISR (TIMER1_CAPT_vect)
{
  static unsigned long prevTimeStamp;
  static unsigned long currentTimeStamp;
  static unsigned int channel = 0;

  unsigned int timer1CounterValue = ICR1;     // save timestamp
  
  unsigned long overflowCopy = overflowCount; // save overflowCount in case that ovf can appear while we mess with this code

  // if just missed an overflow
  // 0x7FFF is magic - have to think about this.
  if (IsLostInterrupt && timer1CounterValue < 0x7FFF)
  {
    overflowCopy++;
  }

  currentTimeStamp = (overflowCopy << 16) + timer1CounterValue; // low 16 bits - counter value, high 16 bits - overflows

  unsigned long value = currentTimeStamp - prevTimeStamp;       // time between 2 pulses  
  if (channel >= channelAmount || value >= PPM_FrGap)           // if we already read all the channels or we expecting more than exists, do resync
  {
    channel = 0;
  }
  else                                                          // if value is valid channel value, then save it and start waiting next channel
  {    
    input[channel] = value;
    channel++;
  }

  prevTimeStamp = currentTimeStamp;                             // save last pulse timestamp
}

