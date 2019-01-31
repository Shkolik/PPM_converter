// Interrupt variables
volatile unsigned long fall_Time = 0;                   // Placeholder for microsecond time when last falling edge occured.
volatile unsigned long rise_Time = 0;                   // Placeholder for microsecond time when last rising edge occured.
volatile byte dutyCycle = 0;                            // Duty Cycle %
volatile unsigned long lastRead = 0;                    // Last interrupt time (needed to determine interrupt lockup due to 0% and 100% duty cycle)

void PinChangeISR0(){                                   // Pin 2 (Interrupt 0) service routine
  lastRead = micros();                                  // Get current time
  if (digitalRead(2) == LOW) {
    // Falling edge
    fall_Time = lastRead;                               // Just store falling edge and calculate on rising edge
  }
  else {
    // Rising edge
    unsigned long total_Time = rise_Time - lastRead;    // Get total cycle time
    unsigned long on_Time = fall_Time - rise_Time;      // Get on time during this cycle
    total_Time = total_Time / on_Time;                  // Divide it down
    dutyCycle = 100 / total_Time;                       // Convert to a percentage
    rise_Time = lastRead;                               // Store rise time
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(2,INPUT);
  Serial.println(F("ISR Pin 2 Configured For Input."));
  attachInterrupt(0,PinChangeISR0,CHANGE);
  Serial.println(F("Pin 2 ISR Function Attached."));
}

void loop() {
  static unsigned long oldLastRead = lastRead;
  Serial.print("Duty Cycle = ");
  if (oldLastRead != lastRead) {
    Serial.print(dutyCycle);
    oldLastRead = lastRead;
  }
  else { // No interrupt since last read so must be 0% or 100%
    if (digitalRead(2) == LOW){
      Serial.print("0");
    }
    else {
      Serial.print("100");
    }
  }
  Serial.println("%");
  delay(100);
}
