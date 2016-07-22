//---Output Pins---
//The latch pin on shift registers
int LATCH = A1;
//Data clock for the shift registers
int CLOCK_OUTPUT = A2;
//Data pin for the shift registers
int DATA = A0;
//Output enable on the shift registers - active HIGH
int OUTPUT_ENABLE = 23;
//Data clear on the shift registers - active HIGH
int CLEAR = A3;
//Buzzer pin
int BUZZER = 10;
//DEBUG/alarm pin
int YEL_LED = 9;

//---Input Pins---
//A pin each for each button on the clock
int HOUR_BUTTON = 3;
int MINUTE_BUTTON = 2;
int SECOND_BUTTON = 1;
int ALARM_BUTTON = 0;


void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}

//updates all three shift registers at the same time (seconds, minutes, hours)
void updateShiftRegisters()
{
  //must be MSB to go from the bottom up
  digitalWrite(LATCH, LOW);
  shiftOut(DATA, CLOCK_OUTPUT, MSBFIRST, secLED);
  shiftOut(DATA, CLOCK_OUTPUT, MSBFIRST, minLED);
  shiftOut(DATA, CLOCK_OUTPUT, MSBFIRST, hrLED);
  digitalWrite(LATCH, HIGH);
}
