<p align=center>
<a href="https://travis-ci.org/jamesbattersby/esp8266-landing-lights"><img src="https://travis-ci.org/jamesbattersby/esp8266-landing-lights.svg?branch=master" alt="Build Status"></a>
</p>

# Landing Lights
This is a simple project that uses an ESP8266, HCSR04 ultrasonic sensor and a strip of WS2812B LEDs to produce a garage parking sensor.
As you drive towards the garage wall, the LEDs slowly go out and change from green to yellow then red as you get closer.  Eventually, 
the entire LED strip flashes red - indicating that it really is time to stop.

Using MQTT, the landing lights can be linked to the garage door sensors to disable the LEDs when the doors are shut.

The code also includes over-air-update capability.

## Parts List
- Wemos D1 mini
- HC SR04 Ultrasonic sensor
- WB2812B LED strip (1m, 60 LEDs)
- USB Power Supply

## Building The Code
I've used Visual Studio Code and Platformio, but this will work equally well with the Arduino IDE.

### Install The Libraries
Only three additional libraries are required:
- FastLED
- xxtea-lib
- PubSubClient

FastLED is required to drive the LED strip, and xxtea-lib is used to provide some protection for your wifi details.

### Configure Wifi Access
A template wifi config file is provided.  This file will eventaully contain your encrypted wifi details.  Firstly copy the `include/wifiConfig.tmpl` file to `include/wifiConfig.h`

If you do not want to enable wifi, simply set `WIFI` to be `false`, you can also disable the use of MQTT by setting `MQTT` to false.  Depending on what you've enabled, you'll need to set some or all of the following:

- `SSID` - The SSID of your wifi network
- `WIFI_PASSWORD` - The access password
- `ENCRYPTION_KEY` - A key to encrypt your wifi data with.  This is just a random string of data, it can be anything up to 50 characters.
- `MQTT_SERVER` - The address of your MQTT server
- `MQTT_PORT` - The port of your MQTT server
- `MQTT_USERNAME` - User name to access the MQTT server
- `MQTT_PASSWORD` - Password to access the MQTT server

When this is done, you can build and download the code to your Wemos.  Monitoring the serial port as it boots up, you will see that it is printing out the encrypted versions of your wifi data, something like this:

`--Encrypted SSID: 16AFE84BD198AAD3543DBA`<br/>
`--Encrypted password: 991888FBBAECE5DAE1426DFBC871635DA41D4BD8FFBD44AE112DAE163`<br/>
`--Encrypted MQTT username: 47464AB37D`<br/>
`--Encrypted MQTT password: 18421372346324BBAE15638D09A84645B`<br/>

Copy this data and replace your originals in `include/wifiConfig.h`, and change `GENERATE_ENCRYPTED_WIFI_CONFIG` to be `false`.

If you build and download again, you should see your Wemos connect to you wifi network.

`Ready`<br/>
`IP address: 192.168.1.100`<br/>
`Distance: 0`

## Wiring It Up
![Schematic](schematic.png)

It may look a bit odd that the RX line from the Wemos is going to the LED data line.  This is simply because I decided to use GPIO3, which is labelled RX on the Wemos PCB.

## Installing
The only thing that needs care is the installation of the ultrasonic sensor, as this can give false readings from reflections off other objects in the garage.  The first thing is to get a mount / enclosure for it.  There are many available from on-line 3D printing stores and eBay.  I found it best to mount the sensor at number plate height.  I used a standard electrical wall box to hold the Wemos.  I know this is obvious, but put some insulating tape over the screws used to fix the mountings to the wall.

## MQTT
When the distance to the car changes, a message is published to `carDistance`.  The message is in the following format:

`<car number>:<distance>`

The `car number` is hard coded to one, as currently only one sensor is supported.

The code listens to `carDistanceQuery`.  If any message is received on this topic, a distance measurement is sent.  The `garageDoors` topic is also listened to.  If a message is received saying that the door is shut, the LEDs will be disabled.

## Set-up
The biggest issue is random reflections from other things in the garage.  Tweak the `SCALING` value until you get stable reading, then you can set the colour change threshold levels.
