Official ClamClock Software
=======

How to program
--------------

The clock's program is run on an ATtiny88 chip. The chip can be programmed through the six pin breakout on the back of the clock using an ISP programmer - another arduino can also be used as an ISP to program it. You will also need the ATtiny-core firmware package and the RTC library for arduino in order to program the controler. Both of these are included in the git repository in the hardware and libraries folders respectively. The files inside these folders should be added to the coresponding hardware and libraries folders in your /Documents/Arduino directory (create these folders if they do not exist). Once you've done this, set up your Arduino IDE with the settings shown in the 'board_config.png'.

How to use
----------

ClamClock is an alarm clock and, like any other alarm clock, you can set both the time and the alarm. 

###To set the alarm:
Hold down the rightmost push-button for 1 second (until the yellow Alarm LED starts blinking) then release to enter alarm-set mode. Set the alarm by using the left most push button to set the hours, and so on to the right for minutes and seconds. Once the desired time has been entered, press the rightmost (Alarm) push-button to return to regular time telling mode - notice that the yellow Alarm LED is now on, this means the alarm is set. To cancel setting the alarm, hold down the alarm button for longer than a second. 

###To set the time: 
Hold down the rightmost push-button for 3 second (until the second's column of LEDs fully illuminates) then release to enter time-set mode. Now, the leftmost button will toggle through the hours and the middle-left button will toggle through minutes. The middle-right button will reset the seconds to zero and exit time set mode. This means that you should set the clock to be a minute ahead of the clock that it is being set against; then, when the other clock rolls over, press the middle-right button set the clock to your minute and hour settings and zero out the seconds. 

###To toggle the alarm on/off: 
Press the alarm button (rightmost) once for less than 1 second to switch the alarm between on and off. When the alarm is on the yellow alarm LED will be lit.

###To switch to 12hr time: 
Hold down the rightmost push-button for 5 seconds (until all the LEDs light up fully) then release to enter 12hr time. 12hr time reads very similarly to 24hr time on the ClamClock. The only change is that the hour column of LEDs now rolls over after 12 hours and the topmost LED in the hour column is off for AM and on for PM. 

How to read
-----------
ClamClock is a binary clock - it counts in base 2 instead of base 10 (if this is foreign to you, you'll find a lot of info through google). Even without fully understanding binary, the clock is easy to read. A graphic explanation is included in the git repository under the namer 'how_to_read.png'.

The clock keeps track of time in 24hr (military) time by default - this can be changed to a 12hr format.

How the clock works
-------------------
The ClamClock uses a micro-controller, shift registers, and a real-time clock (RTC). The micro-controller reads the time from the RTC and then sends signals to the shift-registers to tell which LEDs to light up. The clock requires a 3v coin cell battery to power the RTC (this battery should last ~10 years) and there should already be one in the clock. To power the LEDs on the clock you'll need a 5v power supply with a barrel jack connector. It is very important that this supply is 5v and that the positive (+) end of the connector is on the inside of the barrel jack (just look on the label of the power supply -- you'll see what we mean). If you use the incorrect power supply you will break your ClamClock.
