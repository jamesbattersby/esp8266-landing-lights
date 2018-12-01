//-----------------------------------------------------------------------------
// Landing Lights
// By James Battersby
//-----------------------------------------------------------------------------

// For encryption
#include <xxtea-lib.h>

// For FastLed
#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>

// For OTA
#include <ArduinoOTA.h>

// Config for LEDs
#define DATA_PIN    3
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    60
#define SCALING 7.5 
#define BRIGHTNESS            10
#define FRAMES_PER_SECOND    120
#define YELLOW_THRESHOLD      40
#define RED_THRESHOLD         20
#define RED_FLASH_THRESHOLD    5
CRGB leds[NUM_LEDS];

// Config for ultrasonic sensor
#define echoPin D7 // Echo Pin
#define trigPin D6 // Trigger Pin

// Config for WiFi
#include "wifiConfig.h"

// Local functions
void setUpWifi();
long getDistance();

//-----------------------------------------------------------------------------
// setUp
//
// Configure the serial port, ultrasonic sensor pins and the FastLED library.
//-----------------------------------------------------------------------------
void setup() 
{
  Serial.begin (9600);
#if WIFI
  setUpWifi();
#endif // WIFI

  // Set up ultrasonic sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // Set up LEDs
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
}

//-----------------------------------------------------------------------------
// getDistance
//
// Trigger a pulse to the ultrasonic sensor, time the response and convert
// to cm (approx).
//-----------------------------------------------------------------------------
long getDistance()
{
  long duration = 0;
  long distance = 0;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  // Calculate the distance (in cm) based on the speed of sound.
  distance = duration / 58.2;
  Serial.printf("Distance %ld\n", distance);
  return distance;
}

//-----------------------------------------------------------------------------
// loop
//
// This is the main loop, which will check the distance every 250ms and up-date
// the LEDs.  It will also check for OTA download if WIFI is enabled.
//-----------------------------------------------------------------------------
void loop() 
{
#if WIFI  
  ArduinoOTA.handle();
#endif // WIFI  
  long distance = getDistance();
  long scaledDistance = distance / SCALING;
  int ledsToLight = NUM_LEDS;
  bool flashMode = false;
  static bool state = false;
  static int prevLedsToLight = NUM_LEDS + 1;
  static bool prevFlashMode = true;
  CRGB colour = CRGB(0, 255, 0);

  if (scaledDistance < NUM_LEDS)
  {
    ledsToLight = (int)(scaledDistance);
  }
  if (scaledDistance < YELLOW_THRESHOLD && scaledDistance >= RED_THRESHOLD)
  {
    colour = CRGB(255, 255, 0);
  }
  if (scaledDistance < RED_THRESHOLD)
  {
    colour = CRGB(255, 0, 0);
  }
  if (scaledDistance < RED_FLASH_THRESHOLD)
  {
    flashMode = true;
    state = !state;
    if (state)
    {
      colour = CRGB(255, 0, 0);
    }
    else
    {
      colour = CRGB(0, 0, 0);
    }
    ledsToLight = NUM_LEDS;
  }

  if (prevLedsToLight != ledsToLight || prevFlashMode != flashMode || flashMode)
  {
    fill_solid(&(leds[0]), ledsToLight, colour);
    fill_solid(&(leds[ledsToLight]), NUM_LEDS - ledsToLight, CRGB(0, 0, 0));
    FastLED.show();  
  }
  FastLED.delay(250);
  prevLedsToLight = ledsToLight;
  prevFlashMode = flashMode;
}

//-----------------------------------------------------------------------------
// setUpWifi
//
// Responsible for connecting to Wifi, initialising the over-air-download
// handlers.
// 
// If GENERATE_ENCRYPTED_WIFI_CONFIG is set to true, will also generate
// the encrypted wifi configuration data.
//-----------------------------------------------------------------------------
void setUpWifi()
{
  String ssid = SSID;
  String password = WIFI_PASSWORD;
  
  // Set the key
  xxtea.setKey(ENCRYPTION_KEY);

  // Perform Encryption on the Data
#if GENERATE_ENCRYPTED_WIFI_CONFIG
  Serial.printf("--Encrypted SSID: %s\n", xxtea.encrypt(SSID).c_str());
  Serial.printf("--Encrypted password: %s\n", xxtea.encrypt(WIFI_PASSWORD).c_str());
#endif //GENERATE_ENCRYPTED_WIFI_CONFIG

  unsigned char pw[MAX_PW_LEN];
  unsigned char ss[MAX_PW_LEN];
  // Connect to Wifi
  WiFi.mode(WIFI_STA);
  xxtea.decrypt(password).getBytes(pw, MAX_PW_LEN);
  xxtea.decrypt(ssid).getBytes(ss, MAX_PW_LEN);

  WiFi.begin((const char*)(ss), (const char*)(pw));
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  
}