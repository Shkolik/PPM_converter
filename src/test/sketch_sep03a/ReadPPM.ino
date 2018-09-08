#ifdef __AVR_ATmega16__
  #define IsLostInterrupt (TIFR & bit (TOV1))
#else   
  #define IsLostInterrupt (TIFR1 & bit (TOV1))
#endif

void setupReader()
{
  overflowCount = 0;                        // Therefore no overflows yet

  TCCR1A = 0;                               // set entire TCCR1 register to 0
  TCCR1B = 0;  
  TCCR1B = bit (CS11) | bit (ICES1);        // 8 prescaler plus Input Capture Edge Select (rising on D8)
  TCNT1 = 0;                                // Counter to zero
  
  #ifdef __AVR_ATmega16__
    //TIFR = bit (ICF1) | bit (TOV1);       // clear flags so we don't get a bogus interrupt
    TIMSK |= bit (TICIE1);                  // interrupt on Timer 1 input capture
  #else    
    TIFR1 = bit (ICF1) | bit (TOV1);        // clear flags so we don't get a bogus interrupt    
    TIMSK1 = bit (ICIE1);                   // interrupt on Timer 1 input capture
  #endif
}

//Input capture interrupt
ISR (TIMER1_CAPT_vect)
{
  static unsigned int prevTimeStamp;
  static byte channel = 0;

  unsigned int timer1CounterValue = ICR1;     // save timestamp
       
  unsigned int value;
  if(timer1CounterValue <= prevTimeStamp)     // time between 2 pulses
  { 
    value = 0xFFFF - prevTimeStamp + timer1CounterValue + 1;
    //Serial.println(String(timer1CounterValue) + " " + String(ICR1) + " " + String(prevTimeStamp));
  }
  else 
  {
    value = timer1CounterValue - prevTimeStamp; 
  }

  //Serial.println(String(channel) + " " + String(value));
    
  if (channel >= channelAmount || value >= PPM_FrGap)           // if we already read all the channels or we expecting more than exists, do resync
  {
    channel = 0;
  }
  else                                                          // if value is valid channel value, then save it and start waiting next channel
  {    
    input[channel] = value;
    channel++;
  }

  prevTimeStamp = timer1CounterValue;                             // save last pulse timestamp
  
}

