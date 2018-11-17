<p align=center>
<a href="https://travis-ci.org/jamesbattersby/esp8266-landing-lights"><img src="https://travis-ci.org/jamesbattersby/esp8266-landing-lights.svg?branch=master" alt="Build Status"></a>
</p>

# Landing Lights

This is a simple project that uses an ESP8266, HCSR04 ultrasonic sensor and a strip of WS2812B LEDs to produce a garage parking sensor.  As you
drive towards the garage wall, the LEDs slowly go out and change from green to yellow then red as you get closer.  Eventually, the entire LED
strip flashes red - indicating that it really is time to stop.

The code also includes over-air-update capability.

ReadMe todo:
- add circuit diagram
- add info about parameters to adjust
- add source of information

