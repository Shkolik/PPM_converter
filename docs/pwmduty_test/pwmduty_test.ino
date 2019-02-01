// Interrupt variables
volatile unsigned long fall[2];                   // Placeholder for microsecond time when last falling edge occured.
volatile unsigned long dutyCycle = 0;                            // Duty Cycle %
volatile unsigned long rise[2];                    // Last interrupt time (needed to determine interrupt lockup due to 0% and 100% duty cycle)
volatile unsigned long period = 0;


void PinChangeISR0(){                                   // Pin 2 (Interrupt 0) service routine
  unsigned long lastRead = micros();                                  // Get current time
  if (digitalRead(2) == LOW) {
    // Falling edge
    fall[0] = fall[1];
    fall[1] = lastRead;
    }
  else {
    // Rising edge
    

    period = lastRead - rise[1];
    Serial.print("period ");
    Serial.println(period);
    unsigned long onTime = lastRead - fall[1];
    if(onTime > period)
    {
      onTime -= period;
    }
        Serial.print("OnTime ");
    Serial.println(onTime);

    dutyCycle = onTime*1000/period;
    Serial.print("dutyCycle ");
    Serial.println(dutyCycle);   
    Serial.println();    
    rise[0] = rise[1];
    rise[1] = lastRead;                    
  }
}

void setup() {
  
  Serial.begin(115200);
  pinMode(2,INPUT);
  Serial.println(F("ISR Pin 2 Configured For Input."));
  attachInterrupt(digitalPinToInterrupt(2),PinChangeISR0,CHANGE);
  Serial.println(F("Pin 2 ISR Function Attached."));
  //pwmBegin(32);                     // 250kHz PWM, range for duty is 1-63
}

void loop() {
   /*
    oldLastRead = lastRead;
    Serial.print("Duty Cycle = ");
    Serial.print(dutyCycle);
    Serial.println("");
  
  else { // No interrupt since last read so must be 0% or 100%
    if (digitalRead(2) == HIGH){
      Serial.print("0");
    }
    else {
      Serial.print("100");
    }
  }*/
  
  //delay(100);
}
