/*
  @authors: Travis Libsack (libsackt@mit.edu) and Josef Biberstein (jxb@mit.edu)
  @date: 7/3/2016
  @company: Limbeck Engineering LLC

  Software compiled for use with ClamClock -- a binary timekeeper. This code uses
  open source libraries and itself open and free to the public

  The MIT License (MIT)
  Copyright (c) <2016> <Limbeck Engineering LLC>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:


  Support necessary: ATtiny support, specifically for the ATtiny88


  _snooze def

  _don't blink debug LED for mode - instead light up seconds in time set and blink LED in alarm set

  _time set from current time

  _cycle modes as you hold down alarm button, not when release

  _tap to toggle alarm, hold to enter each setting mode for different times

  _alarm turns off with any button

  hard alarm method? game? add columns?

  code based alarm settings

  find a way to switch to 12 hour time


*/

/*
   INSTRUCTIONS::
   - To Set the TIME:

*/

//Include the Wire and RTC libraries to talk to our Real Time Clock
#include <RTClib.h>
#include <Wire.h>

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

//The three bytes which will be written to the shift registers to show on the LEDs
int hrLED = 0;
int minLED = 0;
int secLED = 0;

//An object representing the Real Time Clock
RTC_DS1307 RTC;

//Time at which the alarm will go off
int alarmHour = NULL;
int alarmMinute = NULL;
int alarmSecond = NULL;


//Enumeration on the state of the clock - defines the clocks behavior
enum STATE {
  SETUP,
  CLOCK,
  SET_TIME,
  SET_ALARM
};
enum STATE state = SETUP;

//Values and prev values of the button pins
int hourButtonState = LOW;
int minuteButtonState = LOW;
int secondButtonState = LOW;
int alarmButtonState = LOW;

int hourButtonStatePrev = LOW;
int minuteButtonStatePrev = LOW;
int secondButtonStatePrev = LOW;
int alarmButtonStatePrev = LOW;

//Flags for button presses. It is the responsibility of the handler to reset this state when it is
//handling a press.
//hour, minute, or second button pressed and released
boolean hourButtonPressedReleased = false;
boolean secondButtonPressedReleased = false;
boolean minuteButtonPressedReleased = false;

//alarm button depressed
boolean alarmButtonPressed = false;
//alarm button held for more than 1 second
boolean alarmButtonHeld = false;
//alarm button held for more than 3 seconds
boolean alarmButtonHeldLong = false;
//alarm button held for more than 5 seconds
boolean alarmButtonHeldVeryLong = false;
//alarm button released
boolean alarmButtonReleased = false;

//Timer for testing whether the alarm button was held or just pressed
unsigned long alarmPressStart = NULL;

//Values used to store the users selections for hours and minutes while setting the time or alarm - set to 0 otherwise
int adjHours = 0;
int adjMins = 0;
int adjSecs = 0;

//State of the debug LED. Used to flash it.
int debugLEDState = LOW;

//Timer used to flash the debug LED
int debugLEDTimer = 0;

//This determines how long the alarm has been going on
//TODO make the duration of the alarm setable
int alarmTimer = 0;

//determines if the alarm will turn on when the alarm time is reached
boolean alarmToggle = false;

//determines whether we display in 24-hour time
boolean twentyFourHourTime = true;

//The current alarm time
void setup()
{
  //set outputs
  pinMode(LATCH, OUTPUT);
  pinMode(DATA, OUTPUT);
  pinMode(CLOCK_OUTPUT, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(OUTPUT_ENABLE, OUTPUT); //output enable on shift registers, active HIGH
  pinMode(CLEAR, OUTPUT); //used for clearing all registers, active HIGH
  pinMode(YEL_LED, OUTPUT); //debugging LED

  //set inputs
  pinMode(HOUR_BUTTON, INPUT);
  pinMode(MINUTE_BUTTON, INPUT);
  pinMode(SECOND_BUTTON, INPUT);
  pinMode(ALARM_BUTTON, INPUT);

  //initialize RTC device
  RTC.begin();

  //enable the shift registers
  digitalWrite(OUTPUT_ENABLE, HIGH);
  digitalWrite(CLEAR, HIGH);

  if (! RTC.isrunning()) {
    //Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // July 14, 2016 at 15:10 you would call:
    //RTC.adjust(DateTime(2016, 7, 14, 15,10,0));
  }

  //put the clock in the CLOCK state
  state = CLOCK;

}


void loop()
{

  //get the current time
  DateTime now = RTC.now();

  //update button states
  updateButtonStates();

  switch (state) {
    case CLOCK:
      //Send time to shift registers
      hrLED = now.hour();
      minLED = now.minute();
      secLED = now.second();

      //Handle the possability of 12-hour time
      if(twentyFourHourTime){
        updateShiftRegisters();
      }else{
        if(hrLED > 12){
          //if the hours are greater than 12, then subtract them from 12 and OR with 16 to light up the top LED.
          hrLED = (hrLED - 12) | 16;
        }
        updateShiftRegisters();
      }


      //check for alarm press
      if (alarmButtonPressed) {
        //run a loop which cycles through each mode we can enter and only transitions once the alarm button is released
        while (!alarmButtonReleased) {
          updateButtonStates();
          if (alarmButtonHeld) {
            hrLED = 0;
            minLED = 0;
            secLED = 0;
            updateShiftRegisters();

            //Cycle DEBUG LED at period of 1 seconds
            if (millis() - debugLEDTimer > 500) {
              writeToDebugLED(debugLEDState == HIGH ? LOW : HIGH);
              debugLEDTimer = millis();
            }
            //WARNING: DO NOT REMOVE
            //this second check is here to anticipate the rolling over of the millis() method after 50 days at which time
            //this subtraction might become negative.
            else if (millis() - debugLEDTimer < 0) {
              debugLEDTimer = 0;
            }

          } else if (alarmButtonHeldLong) {
            //set the time from the current time
            adjHours = now.hour();
            adjMins = now.minute();

            hrLED = adjHours;
            minLED = adjMins;
            secLED = 255;
            updateShiftRegisters();

          } else if (alarmButtonHeldVeryLong) {
            hrLED = 255;
            minLED = 255;
            secLED = 255;
            updateShiftRegisters();
          }

        }

        //transition to another state or not
        if (alarmButtonHeld) {
          state = SET_ALARM;
        } else if (alarmButtonHeldLong) {
          state = SET_TIME;
        } else if (alarmButtonHeldVeryLong) {
          twentyFourHourTime = twentyFourHourTime == true ? false : true;
        } else {
          //toggle alarm
          alarmToggle = alarmToggle == true ? false : true;
          writeToDebugLED(alarmToggle == true ? HIGH : LOW);
        }
      }
      //or check if the alarm has gone off.
      else if (hrLED == alarmHour && minLED == alarmMinute && secLED == alarmSecond && alarmToggle) {
        alarmTriggered();
      }
      break;
    case SET_ALARM:
      //Display current selection
      hrLED = adjHours;
      minLED = adjMins;
      secLED = adjSecs;
      updateShiftRegisters();

      //Cycle DEBUG LED at period of 1 seconds
      if (millis() - debugLEDTimer > 500) {
        writeToDebugLED(debugLEDState == HIGH ? LOW : HIGH);
        debugLEDTimer = millis();
      }
      //WARNING: DO NOT REMOVE
      //this second check is here to anticipate the rolling over of the millis() method after 50 days at which time
      //this subtraction might become negative.
      else if (millis() - debugLEDTimer < 0) {
        debugLEDTimer = 0;
      }

      //Update selections
      adjHours += hourButtonPressedReleased ? 1 : 0;
      adjMins += minuteButtonPressedReleased ? 1 : 0;
      adjSecs += secondButtonPressedReleased ? 1 : 0;

      //Account for wrap around
      adjHours = adjHours > 24 ? 0 : adjHours;
      adjMins = adjMins > 60 ? 0 : adjMins;
      adjSecs = adjSecs > 60 ? 0 : adjSecs;

      //Check if we are setting
      if (!alarmButtonHeld && alarmButtonReleased) {
        //Set alarm
        alarmHour = adjHours;
        alarmMinute = adjMins;
        alarmSecond = adjSecs;
        //reset adjusters
        adjHours = 0;
        adjMins = 0;
        adjSecs = 0;
        state = CLOCK;

        //high becasue we set the alarm
        writeToDebugLED(HIGH);
        alarmToggle = true;
      }
      //or canceling.
      else if (alarmButtonHeld && alarmButtonReleased) {
        //reset adjusters
        adjHours = 0;
        adjMins = 0;
        adjSecs = 0;
        state = CLOCK;

        //low because we did not or unset the alarm
        writeToDebugLED(LOW);
        alarmToggle = false;
      }
      break;
    case SET_TIME:
      //Display current selection
      hrLED = adjHours;
      minLED = adjMins;
      //set seconds to full on in order to indicate that we have entered time set mode
      secLED = 255;
      updateShiftRegisters();

      //Update selections
      adjHours += hourButtonPressedReleased ? 1 : 0;
      adjMins += minuteButtonPressedReleased ? 1 : 0;

      //Account for wrap around
      adjHours = adjHours > 24 ? 0 : adjHours;
      adjMins = adjMins > 60 ? 0 : adjMins;

      //Check if we are setting
      if (secondButtonPressedReleased) {
        RTC.adjust(DateTime(1963, 11, 22, adjHours, adjMins, 0));
        adjHours = 0;
        adjMins = 0;
        state = CLOCK;
      }
      //or canceling.
      else if (alarmButtonReleased) {
        adjHours = 0;
        adjMins = 0;
        state = CLOCK;
      }
      break;
    case SETUP:
      state = CLOCK;
      break;
  }

  resetAllButtonStates();

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

//Gets the current states of all the buttons and updates various flags relating
//to press events- called at begining of loop for a refresh
void updateButtonStates() {
  //setting states
  hourButtonStatePrev = hourButtonState;
  minuteButtonStatePrev = minuteButtonState;
  secondButtonStatePrev = secondButtonState;
  alarmButtonStatePrev = alarmButtonState;

  hourButtonState = digitalRead(HOUR_BUTTON);
  minuteButtonState = digitalRead(MINUTE_BUTTON);
  secondButtonState = digitalRead(SECOND_BUTTON);
  alarmButtonState = digitalRead(ALARM_BUTTON);

  //True if a press has occured
  hourButtonPressedReleased = hourButtonState == LOW && hourButtonStatePrev == HIGH;
  minuteButtonPressedReleased = minuteButtonState == LOW && minuteButtonStatePrev == HIGH;
  secondButtonPressedReleased = secondButtonState == LOW && secondButtonStatePrev == HIGH;

  //The alarm button has been depressed, so start a timer.
  if (alarmButtonState == HIGH && alarmButtonStatePrev == LOW) {
    alarmPressStart = millis();
    alarmButtonPressed = true;
    alarmButtonHeld = false;
    alarmButtonHeldLong = false;
    alarmButtonHeldVeryLong = false;
    alarmButtonReleased = false;
  }
  //handle alarm button being held
  else if (alarmButtonState == HIGH && alarmButtonStatePrev == HIGH) {
    alarmButtonPressed = false;
    alarmButtonReleased = false;
    alarmButtonHeld = false;
    alarmButtonHeldLong = false;
    alarmButtonHeldVeryLong = false;

    unsigned long diff = millis() - alarmPressStart;
    if (diff > 1000 && diff < 3000) {
      alarmButtonHeld = true;
    } else if (diff >= 3000 && diff < 5000) {
      alarmButtonHeldLong = true;
    } else if (diff >= 5000) {
      alarmButtonHeldVeryLong = true;
    }
  }
  //handle alarm being released
  else if (alarmButtonState == LOW && alarmButtonStatePrev == HIGH) {
    alarmButtonReleased = true;
    alarmButtonPressed = false;
    alarmButtonHeld = false;
    alarmButtonHeldLong = false;
    alarmButtonHeldVeryLong = false;

    unsigned long diff = millis() - alarmPressStart;
    if (diff > 1000 && diff < 3000) {
      alarmButtonHeld = true;
    } else if (diff >= 3000 && diff < 5000) {
      alarmButtonHeldLong = true;
    } else if (diff >= 5000) {
      alarmButtonHeldVeryLong = true;
    }
    alarmPressStart = NULL;
  }

}

//Reset all the button states to false. We call this at the end of the loop as we assume all button inputs have
//been processed. Kind of a paranoia method
void resetAllButtonStates() {
  hourButtonPressedReleased = false;
  secondButtonPressedReleased = false;
  minuteButtonPressedReleased = false;
  alarmButtonPressed = false;
  alarmButtonHeld = false;
  alarmButtonHeldLong = false;
  alarmButtonReleased = false;
}

//Use this instead of just a digitalWrite as it also updates the program's record of the LED state
void writeToDebugLED(int state) {
  digitalWrite(YEL_LED, state);
  debugLEDState = state;
}

//This will be called when the alarm is detected - runs the LEDs and the alarm buzzer
void alarmTriggered() {
  alarmTimer = millis();
  hrLED = 255;
  minLED = 255;
  secLED = 255;
  updateShiftRegisters();
  analogWrite(BUZZER, 125);

  while (millis() - alarmTimer < 300000) {
    //update button states
    updateButtonStates();

    //burn cycles

    
    if (millis() - alarmTimer < 0) {
      break;
    }

    //snooze/off functionality
    if (alarmButtonReleased || hourButtonPressedReleased || minuteButtonPressedReleased || secondButtonPressedReleased) {
      break;
    }

    resetAllButtonStates();

  }

  analogWrite(BUZZER, 0);

}
