#include <PPMReader.h>
#include <InterruptHandler.h>   <-- You may need this on some versions of Arduino

//////////////////////CONFIGURATION///////////////////////////////
#define default_servo_value 1500  //set the default servo value
#define PPM_FrLen 20500  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_PulseLen 300  //set the pulse length
#define onState 0  //set polarity of the pulses: 1 is positive, 0 is negative
#define outPin 5  //set PPM signal output pin on the arduino
#define interruptPin 2 //set digital pin to listen PPM
#define channelAmount 6 //set number of channels, 8 channels max
#define debug false //set true if you want see output in serial port
//////////////////////////////////////////////////////////////////

//Chanels order recieving
String chanelsInput = "AETR1234";
String chanelsOutput = "TAER1234";

unsigned int output[channelAmount];

PPMReader ppm(interruptPin, channelAmount);

void setup() {
  
  if(debug){
    Serial.begin(9600);
  }
  
  pinMode(outPin, OUTPUT);

  digitalWrite(outPin, !onState);  //set the PPM signal pin to the default state (off)

  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;

  OCR1A = 100;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();
}

void loop() {
  // Print latest valid values from all channels
  for (int channel = 1; channel <= channelAmount; ++channel) {
    unsigned long value = ppm.latestValidChannelValue(channel, 0);
    char chName = chanelsInput.charAt(channel - 1);

    int chOutIdx = chanelsOutput.indexOf(chName);

    output[chOutIdx] = value;
    
    if(debug)
    {
      Serial.print(String(chName) + " " + String(value) + " ");
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
    OCR1A = PPM_PulseLen * 2;
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
      OCR1A = (PPM_FrLen - calc_rest) * 2;
      calc_rest = 0;
    }
    else{
      OCR1A = (output[cur_chan_numb] - PPM_PulseLen) * 2;
      if(debug){
        Serial.print(String(output[cur_chan_numb]) + " ");
      }
      calc_rest = calc_rest + output[cur_chan_numb];
      cur_chan_numb++;
    }     
  }
}
