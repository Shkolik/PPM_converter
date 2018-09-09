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

const bool debug = false;

const byte channelAmount = 6;                             // set number of channels, 8 channels max

//Chanels order recieving AETR1234
//Chanels order transmitting TAER1234
const unsigned int chanelsOutput[] = {2, 0, 1, 3, 4, 5, 6, 7};

volatile unsigned int input[channelAmount];               // channels readed
volatile unsigned int output[channelAmount];              // channels to translate

void setup()
{
  if (debug)
  {
    Serial.begin(9600);
  }

  pinMode(outPin, OUTPUT);                                // set output pin

  digitalWrite(outPin, !onState);                         // set the PPM signal pin to the default state (off)

  cli ();                                                 // protected code

#ifdef __AVR_ATmega16__
  TCCR0 = 0;                                              // disable Timer0
  TIMSK &= ~(bit(OCIE0) | bit(TOIE0));
#else
  TCCR0A = 0;                                             // disable Timer0
  TCCR0B = 0;
  TIMSK0 = 0;
#endif

  for (byte i = 0; i < channelAmount; i++)                // write default servo values in input and output arrays
  {
    input[i] = default_servo_value;
    output[i] = default_servo_value;
  }
  
  setupReader();                                          // setup PPM reader timers

  setupWriter();                                          // setup PPM writer timers

  sei ();                                                 // start interrupts handling
}

void loop()
{  
  // Get latest valid values from all channels
  for (byte channel = 0; channel < channelAmount; channel++)
  {
    // translate AETR1234 to TAER1234
    unsigned int value = input[chanelsOutput[channel]] - PPM_PulseLen; 

    // if value out of bounds - constrain it and write into output
    output[channel] = constrain(value, min_servo_value - PPM_PulseLen, max_servo_value - PPM_PulseLen);

    if (debug)
    {
      Serial.print(String(value + PPM_PulseLen) + " ");
    }
  }
  if (debug)
  {
    Serial.println();
  }
}
