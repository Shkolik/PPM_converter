#include <PPMReader.h>
#include <InterruptHandler.h>   <-- You may need this on some versions of Arduino

//////////////////////CONFIGURATION///////////////////////////////
#define default_servo_value 1500  //set the default servo value
#define PPM_FrLen 20500  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_PulseLen 300  //set the pulse length
#define onState 0  //set polarity of the pulses: 1 is positive, 0 is negative
#define outPin 5 //set PPM signal output pin on the arduino
#define interruptPin 10 //set digital pin to listen PPM //Atmega16
//#define interruptPin 2 //set digital pin to listen PPM //Arduino
#define channelAmount 6 //set number of channels, 8 channels max

//#define TIMSK1 TIMSK // override timer1 register for Atmega16

//Chanels order recieving AETR1234
//Chanels order transmitting TAER1234
const unsigned int chanelsOutput[] = {2,0,1,3,4,5,6,7};

const boolean debug = false; //set true if you want see output in serial port
//////////////////////////////////////////////////////////////////

unsigned int output[channelAmount];

PPMReader ppm(interruptPin, channelAmount);

void setup() {

  for(int i = 0; i < channelAmount; i++)
  {
    output[i] = default_servo_value;
    }
  
  if(debug){
    Serial.begin(9600);
  }
  
  pinMode(outPin, OUTPUT);

  digitalWrite(outPin, !onState);  //set the PPM signal pin to the default state (off)

  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;

  OCR1A = 1000;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz, 1 microsecond at 8mhz (Atmega16L)
  TIMSK |= (1 << OCIE1A); // enable timer compare interrupt
  //TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt on arduino
  sei();
}

void loop() {
    
  // Get latest valid values from all channels
  for (int channel = 1; channel <= channelAmount; ++channel) {
    unsigned long value = ppm.latestValidChannelValue(channel, default_servo_value);

    output[chanelsOutput[channel - 1]] = value;
    
    if(debug)
    {
      Serial.print(String(value) + " ");
    }
  }
  if(debug)
  {
    Serial.println();
  }
}

ISR(TIMER1_COMPA_vect){ 
  static boolean state = true;
  
  TCNT1 = 0;
  
  //start pulse
  if(state) { 
    digitalWrite(outPin, onState);
    OCR1A = PPM_PulseLen;
    state = false;
    if(debug){
      Serial.println();
    }
  }
  else{  //end pulse and calculate when to start the next pulse
    static byte cur_chan_numb;
    static unsigned int calc_rest;
  
    digitalWrite(outPin, !onState);
    state = true;
  
    if(cur_chan_numb >= channelAmount){
      cur_chan_numb = 0;
      calc_rest = calc_rest + PPM_PulseLen;// 
      OCR1A = (PPM_FrLen - calc_rest);
      calc_rest = 0;
    }
    else{
      OCR1A = (output[cur_chan_numb] - PPM_PulseLen);
      if(debug){
        Serial.print(String(output[cur_chan_numb]) + " ");
      }
      calc_rest = calc_rest + output[cur_chan_numb];
      cur_chan_numb++;
    }     
  }
}
