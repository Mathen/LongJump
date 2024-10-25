#include "LcdInterface.hpp"
#include "Grid.hpp"
#include <LiquidCrystal.h>
#include <ArduinoMqttClient.h>
#include <FastLED.h>
#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_AVR_UNO_WIFI_REV2)
  #include <WiFiNINA.h>
#elif defined(ARDUINO_SAMD_MKR1000)
  #include <WiFi101.h>
#elif defined(ARDUINO_ARCH_ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ARDUINO_PORTENTA_H7_M7) || defined(ARDUINO_NICLA_VISION) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_GIGA) || defined(ARDUINO_OPTA)
  #include <WiFi.h>
#elif defined(ARDUINO_PORTENTA_C33)
  #include <WiFiC3.h>
#elif defined(ARDUINO_UNOR4_WIFI)
  #include <WiFiS3.h>
#endif

#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;    // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

// To connect with SSL/TLS:
// 1) Change WiFiClient to WiFiSSLClient.
// 2) Change port value from 1883 to 8883.
// 3) Change broker value to a server with a known SSL/TLS root certificate 
//    flashed in the WiFi module.

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "longjump.ip-dynamic.org";
int        port     = 1883;
const char topic[]  = "board1/to/board2";

const long interval = 1000;
unsigned long previousMillis = 0;

int count = 0;

// LCD init
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

int buttons[9] = {0};

CRGB leds[9];  // LED array

Grid grid;

void setup()
{
  // put your setup code here, to run once:

  //Pins being used
  //Set Pins not being used to input (to not accidently make a short)
  pinMode( 0, OUTPUT);  // None
  pinMode( 1, OUTPUT);  // None
  pinMode( 2, OUTPUT);  // None
  pinMode( 3, INPUT);   // None
  pinMode( 4, INPUT);   // None
  pinMode( 5, INPUT);   // None
  pinMode( 6, INPUT);   // None
  pinMode( 7, INPUT);   // None
  pinMode( 8, INPUT);   // None
  pinMode( 9, INPUT);   // None
  pinMode(10, INPUT);   // None
  pinMode(11, INPUT);   // None
  pinMode(12, INPUT);   // None
  pinMode(13, INPUT);   // None
  pinMode(A0, OUTPUT);  // Out: grid piece placement row[0]
  pinMode(A1, OUTPUT);  // Out: grid piece placement row[1]
  pinMode(A2, OUTPUT);  // Out: grid piece placement row[2]
  pinMode(A3, INPUT);   //  In: grid piece placement col[0]
  pinMode(A4, INPUT);   //  In: grid piece placement col[1]
  pinMode(A5, INPUT);   //  In: grid piece placement col[2]

  // Initialize LEDs
  FastLED.addLeds<WS2812, 6, GRB>(leds, 9);
  FastLED.setBrightness(50);

  //Grid
  grid = Grid(3);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Welcome! Go to:");
  // Move to bottom row
  lcd.setCursor(0,1);
  lcd.print("t.ly/h7NLS");

  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  // attempt to connect to WiFi network:
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }

  Serial.println("You're connected to the network");
  Serial.println();

  // You can provide a unique client ID, if not set the library uses Arduino-millis()
  // Each client must have a unique client ID
  // mqttClient.setId("clientId");

  // You can provide a username and password for authentication
  // mqttClient.setUsernamePassword("username", "password");
  delay(5000);

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
}

void loop()
{
  // put your main code here, to run repeatedly:
  //grid.Update();

  // call poll() regularly to allow the library to send MQTT keep alives which
  // avoids being disconnected by the broker
  mqttClient.poll();

  // to avoid having delays in loop, we'll use the strategy from BlinkWithoutDelay
  // see: File -> Examples -> 02.Digital -> BlinkWithoutDelay for more info
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval)
  {

    mqttClient.beginMessage(topic);
    for (int i = 0; i < 9; i++)
    {
      Serial.print(buttons[i]);
      Serial.print(" ");

      if (i % 3 == 0)
        mqttClient.println();
      if (buttons[i] > 100)
        mqttClient.print("1 ");
      else
        mqttClient.print("0 ");

    }
    mqttClient.endMessage();
    Serial.println();

    // save the last time a message was sent
    previousMillis = currentMillis;

    Serial.print("Sending message to topic: ");
    Serial.println(topic);
    Serial.print("hello ");
    Serial.println(count);

    // send message, the Print interface can be used to set the message contents
    mqttClient.beginMessage(topic);
    mqttClient.print("hello ");
    mqttClient.print(count);
    mqttClient.endMessage();

    Serial.println();

    count++;
  }

    // Test top row
  pinMode(A0, OUTPUT); 
  digitalWrite(A0, HIGH);  
  buttons[0] = analogRead(A3);
  buttons[1] = analogRead(A4);
  buttons[2] = analogRead(A5);
  digitalWrite(A0, LOW);   
  pinMode(A0, INPUT);
  delay(10);

  // Test middle row
  pinMode(A1, OUTPUT); 
  digitalWrite(A1, HIGH);  
  buttons[3] = analogRead(A3);
  buttons[4] = analogRead(A4);
  buttons[5] = analogRead(A5);
  digitalWrite(A1, LOW);   
  pinMode(A1, INPUT); 
  delay(10); 

  // Test bottom row
  pinMode(A2, OUTPUT); 
  digitalWrite(A2, HIGH);  
  buttons[6] = analogRead(A3);
  buttons[7] = analogRead(A4);
  buttons[8] = analogRead(A5);
  digitalWrite(A2, LOW);   
  pinMode(A2, INPUT); 
  delay(10); 

    for (int i = 0; i < 9; i++) {
      if (buttons[i] > 100) {
        leds[i] = CHSV((i * 255 / 9), 255, 255);
      }
      else {
        leds[i] = CRGB::Black;
      }
    }

  FastLED.show();

  delay(10);  // Small delay to avoid bouncing 
}