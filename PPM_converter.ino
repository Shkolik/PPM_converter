//////////////////////CONFIGURATION///////////////////////////////
#define default_servo_value 1500  //set the default servo value
#define min_servo_value 950  //set the min servo value
#define max_servo_value 2050  //set the max servo value

#define PPM_FrGap 3000  //set the gap between PPM frames microseconds (1ms = 1000µs)
#define PPM_FrLen 20500  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_PulseLen 300  //set the pulse length
#define onState 0  //set polarity of the pulses: 1 is positive, 0 is negative

#define outPin 1 //set PPM signal output pin
//use digital pins 8 to 13 on arduino
//or PORTB on ATmega because of dirrect port write

#define interruptPin 10 //set digital pin to listen PPM //Atmega16
//#define interruptPin 2 //set digital pin to listen PPM //Arduino
const byte channelAmount = 6; //set number of channels, 8 channels max

//#define TIMSK1 TIMSK // override timer1 register for Atmega16

//Chanels order recieving AETR1234
//Chanels order transmitting TAER1234
const unsigned int chanelsOutput[] = {2, 0, 1, 3, 4, 5, 6, 7};

const boolean debug = true; //set true if you want see output in serial port
//////////////////////////////////////////////////////////////////
volatile unsigned long input[channelAmount];
volatile unsigned long output[channelAmount];


unsigned long totalMicroseconds;
volatile unsigned long overflowCount;

void setup() {

  //set all channels in middle position
  for (int i = 0; i < channelAmount; i++)
  {
    output[i] = default_servo_value;
    input[i] = default_servo_value;
  }

  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(outPin, OUTPUT);
  digitalWrite(outPin, !onState);  //set the PPM signal pin to the default state (off)

  if (debug) {
    Serial.begin(9600);
  }

  cli();
  //setup int0 to listen ppm input
  GICR |= (1 << INT0);   //Enable INT0
  MCUCR |= (1 << ISC01); //Configure INT0 as falling edge

  //setup timer2
  //Setup Timer2 to fire every 250us
  TCCR2 = 0; // set entire TCCR1 register to 0
  TCNT2 = 0;

  TCCR2 |= (1 << CS21);  // 8 prescaler: 0,5 microseconds at 16mhz, 1 microsecond at 8mhz (Atmega16L)
  TIMSK |= (1 << TOIE2);

  
    //setup Timer1
    TCCR1A = 0; // set entire TCCR1 register to 0
    TCCR1B = 0;

    OCR1A = 100;  // compare match register, change this
    TCCR1B |= (1 << WGM12);  // turn on CTC mode
    TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz, 1 microsecond at 8mhz (Atmega16L)
    TIMSK |= (1 << OCIE1A); // enable timer compare interrupt on arduino
  
  sei();
}

//get total count for Timer2
unsigned long getMicros()
{
  uint8_t SREG_old = SREG; //back up the AVR Status Register;
  noInterrupts(); //prepare for critical section of code
  uint8_t tcnt2_save = TCNT2; //grab the counter value from Timer2
  boolean flag_save = bitRead(TIFR, 6); //grab the timer2 overflow flag value;
  if (flag_save) { //if the overflow flag is set
    tcnt2_save = TCNT2; //update variable just saved since the overflow flag could have just tripped between previously saving the TCNT2 value and reading bit 6 of TIFR.
    //If this is the case, TCNT2 might have just changed from 255 to 0, and so we need to grab the new value of TCNT2 to prevent an error of up
    //to 255us in any time obtained using the T2 counter (ex: T2_micros). (Note: 255 counts /us = 255us)
    overflowCount++;    //force the overflow count to increment
    TIFR |= (1 << TOV2);//reset Timer2 overflow flag since we just manually incremented above; this prevents execution of Timer2's overflow ISR
  }
  totalMicroseconds = overflowCount * 256 + tcnt2_save; //get total Timer2 count
  SREG = SREG_old; //re-enable interrupts if they were enabled before, or to leave them disabled if they were disabled before
  return totalMicroseconds;
}

void loop() {
  
    // Get latest valid values from all channels
    for (byte channel = 0; channel < channelAmount; channel++) {
      unsigned long value = input[chanelsOutput[channel]];

      output[channel] = constrain(value, min_servo_value, max_servo_value);

      if (debug)
      {
        Serial.print(String(channel + 1) + ": " + String(value) + " ");
      }
    }
    if (debug)
    {
      Serial.println();
    }
}



ISR(TIMER2_OVF_vect) // TIMER2 overflow ISR
{
  overflowCount++;
};

ISR (INT0_vect)          //External int0 ISR
{  
  static byte channel;
  static unsigned long microsAtLastPulse = 0;

  // Remember the current micros() and calculate the time since the last pulseReceived()
  unsigned long previousMicros = microsAtLastPulse;
  microsAtLastPulse = getMicros();
  unsigned long pulseTime = microsAtLastPulse - previousMicros;

  //End of frame - start new capture session
  if (pulseTime >= PPM_FrGap || channel >= channelAmount) {
    channel = 0;
  }
  else {
    // Store times between pulses as channel values
    //if (pulseTime >= min_servo_value - 10 && pulseTime <= max_servo_value + 10) {
    input[channel] = pulseTime;
    //}
    ++channel;
  }
}

ISR(TIMER1_COMPA_vect) //TIMER1 compare ISR
{
  static boolean state = true;
  static byte channel;
  static unsigned long calc_rest;
  TCNT1 = 0; //reset counter

  //start pulse
  if (state) {
    digitalWrite(outPin, onState);
    OCR1A = PPM_PulseLen;
    state = false;
  }
  else {
    //end pulse and calculate when to start the next pulse
    digitalWrite(outPin, !onState);
    state = true;

    if (channel >= channelAmount) {
      OCR1A = (PPM_FrLen - calc_rest - PPM_PulseLen);
      calc_rest = 0;
      channel = 0;
    }
    else {
      unsigned long value = output[channel];
      OCR1A = (value - PPM_PulseLen);
      calc_rest += value;
      channel++;
    }
  }
}
