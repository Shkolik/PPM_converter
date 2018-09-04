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
#ifdef __AVR_ATmega16__
ISR(TIMER2_COMP_vect)
#else
ISR(TIMER2_COMPA_vect)
#endif
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

#ifdef __AVR_ATmega16__
  OCR2 = OCR2A_next;
#else
  OCR2A = OCR2A_next;
#endif
}

