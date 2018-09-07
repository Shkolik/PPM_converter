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
  OCR2A = 100;                      // count to 100 (zero-relative)
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
  static unsigned int OCR2_next = OCR2_CorrectedFull;
  static boolean state = true;

  static byte channel = 0;
  unsigned int pulseRest = 0;
  unsigned int frameRest = 0;

  TCNT2 = 0;                                      // reset counter

  if (state)                                                // start pulse
  {
    if (pulseRest == 0)                                // if pulseRest == 0, then it's start of new pulse or it's last iteration in that pulse
    {
      // write pulse polarity
      if (onState)
        OUT_PORT |= outPinBitMask;
      else
        OUT_PORT &= ~outPinBitMask;

      pulseRest = PPM_PulseLen;   // count interrupts

      //branch = 1;
      OCR2_next = OCR2_CorrectedFull;                          // next interrupt after 250 ticks
    }
    else if ( pulseRest > OCR2_MaxCount)                                                // in the middle wait and interrupt every 250 ticks
    {
      //branch = 4;
      OCR2_next = OCR2_CorrectedFull;
    }
    else                                                  // last interrupt in this pulse. Change state after this
    {
      //branch = 3;
      OCR2_next = pulseRest;
      state = false;
    }
  }
  else                                                      // state changed. wait for next channel pulse
  {
    if (channel >= channelAmount)                           // all channels sended. calculate syncro gap
    {
      if (pulseRest == 0)
      {

        if (!onState)
          OUT_PORT |= outPinBitMask;
        else
          OUT_PORT &= ~outPinBitMask;

        pulseRest = frameRest;

        //branch = 5;
        OCR2_next = OCR2_CorrectedFull;                          // next interrupt after 250 ticks
      }
      else if (pulseRest > 250)
      {
        //branch = 8;
        OCR2_next = OCR2_CorrectedFull;
      }
      else                                                  // last interrupt in frame
      {
        //branch = 7;
        OCR2_next = pulseRest;
        state = true;
        channel = 0;
      }
    }
    else
    {
      if (pulseRest == 0)
      {
        if (!onState)
          OUT_PORT |= outPinBitMask;
        else
          OUT_PORT &= ~outPinBitMask;

        pulseRest = output[channel];// - PPM_PulseLen;

        //branch = 9;
        OCR2_next = OCR2_CorrectedFull;                          // next interrupt after 250 ticks
      }
      else if (pulseRest > OCR2_MaxCount)
      {
        //branch = 12;
        OCR2_next = OCR2_CorrectedFull;
      }
      else                                                    // last interrupt in channel
      {
        //branch = 11;
        OCR2_next = pulseRest;
        channel++;
        state = true;
      }
    }
  }

  if (pulseRest > OCR2_MaxCount)                         // decrement pulseRest every interrupt
  {
    pulseRest -= OCR2_MaxCount;
  }
  else
  {
    pulseRest = 0;
  }

  if (frameRest > OCR2_MaxCount)                         // decrement frameRest every interrupt
  {
    frameRest -= OCR2_MaxCount;
  }
  else
  {
    frameRest = PPM_FrLen;
  }

#ifdef __AVR_ATmega16__
  OCR2 = OCR2_next;
#else
  OCR2A = OCR2_next;
#endif
}
