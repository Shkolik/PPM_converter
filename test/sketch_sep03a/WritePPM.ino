void setupWriter()
{
  //setup timer2
  
  #ifdef __AVR_ATmega16__  
    TCCR2 = 0;                        // set entire TCCR2 register to 0
    TCNT2 = 0;                        // reset timer    
    TCCR2 = bit (WGM21) | bit (CS21); // CTC mode and 8 prescaler: 0,5 microseconds at 16mhz, 1 microsecond at 8mhz (Atmega16L)
    OCR2 = OCR2A_full;                // count to 250 (zero-relative)    
    TIMSK |= bit (OCIE2);             // enable compare interrupt    
  #else    
    TCCR2A = 0;                       // set entire TCCR2 register to 0
    TCCR2B = 0;
    TCNT2 = 0;                        // reset timer
    TCCR2A = bit (WGM21);             // CTC
    TCCR2B = bit (CS21);              // 8 prescaler: 0,5 microseconds at 16mhz, 1 microsecond at 8mhz (Atmega16L)
    OCR2A = OCR2A_full;               // count to 250 (zero-relative)
    TIMSK2 |= bit (OCIE2A);           // enable compare interrupt
  #endif
}

//compare interrupt routine
ISR(TIMER2_COMPA_vect)
{
  static unsigned int interruptCount = 0;
  static unsigned int OCR2A_rest = 0;
  static unsigned int OCR2A_next = OCR2A_full;
  static boolean state = true;
  static boolean wait = false;
  static byte channel = 0;
  static unsigned long calc_rest;

  TCNT2 = 0;                                      // reset counter

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
        interruptCount = PPM_PulseLen / (OCR2A_full + 1);   // count interrupts
        if(interruptCount > 0)
        {
          OCR2A_next = OCR2A_full;
          wait = true;
        }
        else
        {
          OCR2A_next = PPM_PulseLen % (OCR2A_full + 1);
          state = false;
          wait = false;
        }                         
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
      OCR2A_rest = PPM_PulseLen % (OCR2A_full + 1);
    }
  }
  else
  {    
    //end pulse and calculate when to start the next pulse
    if (channel >= channelAmount)
    {
      unsigned int channelLength = (PPM_FrLen - calc_rest - PPM_PulseLen);
      if (interruptCount < 1)
      {
        if (!wait)
        {
          interruptCount  = channelLength / (OCR2A_full + 1);
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
        OCR2A_rest = channelLength % (OCR2A_full + 1);
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
          interruptCount = channelLength / (OCR2A_full + 1);
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
        OCR2A_rest = channelLength % (OCR2A_full + 1);
      }
    }
  }

  #ifdef __AVR_ATmega16__
    OCR2 = OCR2A_next;
  #else
    OCR2A = OCR2A_next;
  #endif
}
