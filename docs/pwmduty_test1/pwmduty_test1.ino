#if F_CPU == 16000000L
#define ticksInMicrosecond  2                             //tick count in 1us with 1/8 prescaller on 16MHz
#else
#define ticksInMicrosecond  1                             //tick count in 1us with 1/8 prescaller on 8MHz
#endif

#define writerCorrection (4 * ticksInMicrosecond)         // each ppmwriter IRS takes about 4µs

#define default_servo_value (1500 * ticksInMicrosecond)   // default servo value
#define min_servo_value (950 * ticksInMicrosecond)        // min servo value
#define max_servo_value (2050 * ticksInMicrosecond)       // max servo value

#define PPM_FrGap (2500 * ticksInMicrosecond)             // gap between PPM frames in ticks
#define PPM_FrLen (22000 * ticksInMicrosecond)            // PPM frame length in ticks
#define PPM_PulseLen (300 * ticksInMicrosecond)           // synchro pulse length
#define onState 0                                         // polarity of the pulses: 1 is positive, 0 is negative

#define OCR2_full 249                                     // Max value for OCR2 register
#define OCR2_CorrectedFull (OCR2_full - writerCorrection) // Max corrected value for OCR2 register
#define OCR2_MaxCount (OCR2_full + 1)                     // Max ticks count in OCR2
#define OCR2_CorrectedCount (OCR2_MaxCount - writerCorrection)// Max corrected ticks count in OCR2

// Because of direct port access use PORTB 1-5 as output
// arduino pins 9-13
// or redefine OUT_PORT as you want and use another pins
#ifdef __AVR_ATmega16__
#define outPin 1                                          // PPM signal output pin
#else
#define outPin 12                                         // PPM signal output pin
#endif

#define outPinBitMask digitalPinToBitMask(outPin)         // bit mask for selected output pin
#define OUT_PORT PORTB                                    // output port

const byte channelAmount = 1;                             // set number of channels, 8 channels max

volatile unsigned int output;

const byte pwmInputPin = 2;
volatile byte testState = 0;
volatile word timerValue[4];
float pwmPeriod, pwmWidth, pwmDuty, pwmFrequency;

void setup()
{
   cli ();
   
  pinMode(outPin, OUTPUT);                                // set output pin

  digitalWrite(outPin, !onState);                         // set the PPM signal pin to the default state (off)

  setupWriter();                                          // setup PPM writer timers
  
  TIMSK0 = 0;                      // for testing with timer0 disabled

  sei ();    
  
  pwmMeasureBegin();
  Serial.begin(115200);
}

void loop()
{
  if (testState == 5) {            // tests completed
    noInterrupts();
    word periodValue = timerValue[3] - timerValue[2];
    word widthValue = timerValue[1] - timerValue[2];
    word diffValue = widthValue - periodValue;
    pwmPeriod = periodValue * 0.0625;
    pwmWidth =  (widthValue - (2 * periodValue)) * 0.0625;
    if (pwmWidth > pwmPeriod)
      pwmWidth -= pwmPeriod;
    pwmDuty = (pwmWidth * 1000 / pwmPeriod);// * 100;
    pwmFrequency = 1000 / pwmPeriod;

    output = map(pwmDuty, 300, 1000, 1000, 2000);
    /*
    Serial.print("pwmPeriod     ");
    Serial.print(pwmPeriod, 3);
    Serial.println(" us");

    Serial.print("pwmWidth      ");
    Serial.print(pwmWidth, 3);
    Serial.println(" us");

    Serial.print("pwmDuty       ");
    Serial.print(pwmDuty, 3);
    Serial.println(" %");

    Serial.print("pwmFrequency  ");
    Serial.print(pwmFrequency, 3);
    Serial.println(" kHz");
    Serial.println();*/

    Serial.print("Output     ");
    Serial.print(output);
    Serial.println();
    interrupts();
    
    pwmMeasureBegin ();
  }
}

ISR (TIMER1_CAPT_vect)
{
  switch (testState) {

    case 0:                            // first rising edge
      timerValue[0] = ICR1;
      testState = 1;
      break;

    case 1:                            // second rising edge
      timerValue[1] = ICR1;
      TCCR1B &=  ~bit (ICES1);         // capture on falling edge (pin D8)
      testState = 2;
      break;

    case 2:                            // first falling edge
    //timerValue[2] = ICR1;
      testState = 3;
      break;

    case 3:                            // second falling edge
      timerValue[2] = ICR1;
      testState = 4;
      break;

    case 4:                            // third falling edge
      timerValue[3] = ICR1;
      testState = 5;                   // all tests done
      break;
  }
}
/*
void pwmBegin(unsigned int duty) {
  TCCR2A = 0;                               // TC2 Control Register A
  TCCR2B = 0;                               // TC2 Control Register B
  TIMSK2 = 0;                               // TC2 Interrupt Mask Register
  TIFR2 = 0;                                // TC2 Interrupt Flag Register
  TCCR2A |= (1 << COM2B1) | (1 << WGM21) | (1 << WGM20);  // OC2B cleared/set on match when up/down counting, fast PWM
  TCCR2B |= (1 << WGM22) | (1 << CS20);     // no clock prescaler for maximum PWM frequency
  OCR2A = 63;                               // TOP overflow value is 63 producing 250 kHz PWM
  OCR2B = duty;
  pinMode(3, OUTPUT);
}
*/
void pwmMeasureBegin ()
{
  TCCR1A = 0;                               // normal operation mode
  TCCR1B = 0;                               // stop timer clock (no clock source)
  TCNT1  = 0;                               // clear counter
  TIFR1 = bit (ICF1) | bit (TOV1);          // clear flags
  testState = 0;                            // clear testState
  TIMSK1 = bit (ICIE1);                     // interrupt on input capture
  TCCR1B =  bit (CS10) | bit (ICES1);       // start clock with no prescaler, rising edge on pin D8
}
