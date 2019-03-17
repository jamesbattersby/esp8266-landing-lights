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
#define SCALING 4
#define BRIGHTNESS            10
#define FRAMES_PER_SECOND    120
#define YELLOW_THRESHOLD      40
#define RED_THRESHOLD         20
#define RED_FLASH_THRESHOLD    3
CRGB leds[NUM_LEDS];

// Config for ultrasonic sensor
#define echoPin D7 // Echo Pin
#define trigPin D6 // Trigger Pin

// Config for WiFi
#include "wifiConfig.h"

// For MQTT messaging
#if MQTT
#include <PubSubClient.h>
#endif // MQTT

// Local functions
void setUpWifi();
long getDistance();
void mqttCallback(char*, byte*, unsigned int);
void connectToMqtt();

#if MQTT
WiFiClient espClient;
PubSubClient mqttClient(espClient);
void notify(int);
#endif // MQTT

bool doorOpen = true;
int lastDistance = 0;

//-----------------------------------------------------------------------------
// setUp
//
// Configure the serial port, ultrasonic sensor pins and the FastLED library.
//-----------------------------------------------------------------------------
void setup()
{
  Serial.begin (115200);
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
// the LEDs.  It will also check for OTA download and MQTT messages if WIFI is
// enabled.
//-----------------------------------------------------------------------------
void loop()
{
#if WIFI
  ArduinoOTA.handle();
#if MQTT
  if (!mqttClient.connected())
  {
    connectToMqtt();
  }
  mqttClient.loop();
#endif // MQTT
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

  if (doorOpen == false)
  {
    ledsToLight = 0; // Garage door is shut - turn off the LEDs
  }
  else
  {
#if MQTT
    notify((int)scaledDistance);
#endif // MQTT
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
  lastDistance = (int)scaledDistance;
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
  Serial.printf("--Encrypted Wifi SSID: %s\n", xxtea.encrypt(SSID).c_str());
  Serial.printf("--Encrypted Wifi password: %s\n", xxtea.encrypt(WIFI_PASSWORD).c_str());
  Serial.printf("--Encrypted MQTT username: %s\n", xxtea.encrypt(MQTT_USERNAME).c_str());
  Serial.printf("--Encrypted MQTT password: %s\n", xxtea.encrypt(MQTT_PASSWORD).c_str());
#endif // GENERATE_ENCRYPTED_WIFI_CONFIG

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

#if MQTT
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
#endif // MQTT

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

#if MQTT
//-----------------------------------------------------------------------------
// connectToMqtt
//
// Connect the the MQTT server
//-----------------------------------------------------------------------------
void connectToMqtt()
{
  unsigned char mqttUser[MAX_PW_LEN];
  unsigned char mqttPassword[MAX_PW_LEN];
  String username = MQTT_USERNAME;
  String passwordmqtt = MQTT_PASSWORD;
  xxtea.decrypt(username).getBytes(mqttUser, MAX_PW_LEN);
  xxtea.decrypt(passwordmqtt).getBytes(mqttPassword, MAX_PW_LEN);
  int retry = 20;

  while (!mqttClient.connected() && --retry)
  {
    Serial.println("Connecting to MQTT...");

    if (mqttClient.connect("LandingLights", reinterpret_cast<const char *>(mqttUser), reinterpret_cast<const char *>(mqttPassword)))
    {
      Serial.println("connected");
      mqttClient.subscribe("garageDoors");
      mqttClient.subscribe("carDistanceQuery");
    }
    else
    {
      Serial.print("failed with state ");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }

  if (retry == 0)
  {
    printf("Failed to connect to MQTT server on %s:%d", MQTT_SERVER, MQTT_PORT);
  }
}

//-----------------------------------------------------------------------------
// mqttCallback
//
// Process a received message
//-----------------------------------------------------------------------------
void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  if (length > 2 && !strcmp(topic, "garageDoors"))
  {
    if (payload[0] == 0x31) // Check for door 1
    {
      if (!strncmp((char*)&payload[2], "closed", length - 2))
      {
        doorOpen = false;
      }
      else
      {
        doorOpen = true;
      }
    }
  }
  else if (!strcmp(topic, "carDistanceQuery"))
  {
    notify(lastDistance);
  }
}

//-----------------------------------------------------------------------------
// notify
//
// Send notification of the car distance to the MQTT server, and to the
// serial port.
//-----------------------------------------------------------------------------
void notify(int distance)
{
  char message[20];
  sprintf(message, "1:%d", distance > NUM_LEDS ? NUM_LEDS : distance);
  printf("%s\n", message);
  mqttClient.publish("carDistance", message);
}

#endif // MQTT
