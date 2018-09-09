void setupReader()
{
  TCCR1A = 0;                                   // set entire TCCR1 register to 0
  TCCR1B = 0;
  TCCR1B = bit (CS11) | bit (ICES1);            // 8 prescaler plus Input Capture Edge Select (rising on D8)
  TCNT1 = 0;                                    // Counter to zero

#ifdef __AVR_ATmega16__  
  TIMSK |= bit (TICIE1);                        // interrupt on Timer 1 input capture
#else
  TIMSK1 = bit (ICIE1);                         // interrupt on Timer 1 input capture
#endif
}

ISR (TIMER1_CAPT_vect)                          // Input capture interrupt
{
  static unsigned int prevTimeStamp;
  static byte channel = 0;                      // Channels counter

  unsigned int value;                           // time between 2 pulses
  if (ICR1 <= prevTimeStamp)                    // if overflow ocures use different math
  {
    value = 0xFFFF - prevTimeStamp + ICR1 + 1;
  }
  else
  {
    value = ICR1 - prevTimeStamp;
  }

  prevTimeStamp = ICR1;                         // save last pulse timestamp
                                                // if we already read all the channels or we expecting more than exists, do resync                                                
  if (channel >= channelAmount || value >= PPM_FrGap)           
  {
    channel = 0;
  }
  else                                          // if value is valid channel value, then save it and start waiting next channel
  {
    input[channel] = value;
    channel++;
  }
}

