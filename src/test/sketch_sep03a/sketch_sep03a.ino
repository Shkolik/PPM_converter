#if F_CPU == 16000000L
  #define ticksInMicrosecond  2                          //tick count in 1us with 1/8 prescaller
#else
  #define ticksInMicrosecond  1                          //tick count in 1us with 1/8 prescaller
#endif

#define writerCorrection (4 * ticksInMicrosecond)        // each ppmwriter IRS takes about 4µs

#define default_servo_value (1500 * ticksInMicrosecond)  //set the default servo value
#define min_servo_value (950 * ticksInMicrosecond)       //set the min servo value
#define max_servo_value (2050 * ticksInMicrosecond)      //set the max servo value

#define PPM_FrGap (3000 * ticksInMicrosecond)            //set the gap between PPM frames microseconds (1ms = 1000Вµs)
#define PPM_FrLen (20500 * ticksInMicrosecond)           //set the PPM frame length in microseconds (1ms = 1000Вµs)
#define PPM_PulseLen (300 * ticksInMicrosecond)          //set the pulse length
#define onState 0                                        //set polarity of the pulses: 1 is positive, 0 is negative

#define OCR2_full 249                                   //Max value for OCR2 register
#define OCR2_CorrectedFull (OCR2_full - writerCorrection)                                   //Max value for OCR2 register
#define OCR2_MaxCount (OCR2_full + 1)                     //Max value for OCR2 register
#define OCR2_CorrectedCount (OCR2_MaxCount - writerCorrection)                         //Max value for OCR2 register

// use PORTB 1-5 as output
// arduino pins 9-13
#ifdef __AVR_ATmega16__
#define outPin 1                                       //set PPM signal output pin
#else
#define outPin 12                                      //set PPM signal output pin
#endif

#define outPinBitMask digitalPinToBitMask(outPin)
#define OUT_PORT PORTB

const bool debug = false;

const byte channelAmount = 6;                            //set number of channels, 8 channels max

//Chanels order recieving AETR1234
//Chanels order transmitting TAER1234
const unsigned int chanelsOutput[] = {2, 0, 1, 3, 4, 5, 6, 7};

volatile unsigned int input[channelAmount];             //channels readed
volatile unsigned int output[channelAmount];            //channels to translate

volatile unsigned long overflowCount = 0;
void setup()
{
  if (debug)
  {
    Serial.begin(9600);
  }

  pinMode(outPin, OUTPUT);                              // set output pin

  digitalWrite(outPin, !onState);                       // set the PPM signal pin to the default state (off)

  cli ();                                               // protected code

#ifdef __AVR_ATmega16__
  TCCR0 = 0;                                            // disable Timer0
  TIMSK &= ~(bit(OCIE0) | bit(TOIE0));
#else
  TCCR0A = 0;                                           // disable Timer0
  TCCR0B = 0;
  TIMSK0 = 0;
#endif

  setupReader();                                        // setup PPM reader timers

  setupWriter();                                        // setup PPM writer timers

  sei ();                                               // start interrupts handling  
}

void loop()
{
  // Get latest valid values from all channels
  for (byte channel = 0; channel < channelAmount; channel++)
  {
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
