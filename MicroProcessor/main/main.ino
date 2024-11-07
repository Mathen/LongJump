//#include <FastLED.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiProvisioner.h>
#include <LiquidCrystal.h>
#include <ArduinoMqttClient.h>
#include <WiFi.h>

#include "LcdInterface.hpp"
#include "Grid.hpp"

// To connect with SSL/TLS:
// 1) Change WiFiClient to WiFiSSLClient.
// 2) Change port value from 1883 to 8883.
// 3) Change broker value to a server with a known SSL/TLS root certificate 
//    flashed in the WiFi module.

WiFiProvisioner::WiFiProvisioner provisioner;
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "longjump.ip-dynamic.org";
int        port     = 1883;
const char topic[]  = "board1/to/board2";

const long interval = 1000;
unsigned long previousMillis = 0;

int count = 0;

//LiquidCrystal lcd(31, 37, 30, 28, 27, 23);
LiquidCrystal lcd(19, 23, 18, 17, 16, 15);

Grid grid;

void setup()
{
  //Pins being used
  //Set Pins not being used to input (to not accidently make a short)
  /*
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
  */
  /*
  pinMode(A0, OUTPUT);  // Out: grid piece placement row[0] 
  pinMode(A1, OUTPUT);  // Out: grid piece placement row[1]
  pinMode(A2, OUTPUT);  // Out: grid piece placement row[2]
  pinMode(A3, INPUT);   //  In: grid piece placement col[0]
  pinMode(A4, INPUT);   //  In: grid piece placement col[1]
  pinMode(A5, INPUT);   //  In: grid piece placement col[2]
*/
  // ESP32
  pinMode(33, OUTPUT);  // Out: grid piece placement row[0] //20
  pinMode(32, OUTPUT);  // Out: grid piece placement row[1] //21
  pinMode(25, OUTPUT);  // Out: grid piece placement row[2] //22
  pinMode(34, INPUT);   //  In: grid piece placement col[0] //23
  pinMode(39, INPUT);   //  In: grid piece placement col[1] //24
  pinMode(36, INPUT);   //  In: grid piece placement col[2] //25

  // Initialize LEDs
  //FastLED.addLeds<WS2812, 13, GRB>(leds, 9);
  //FastLED.setBrightness(50);
  // LCD init
  //Grid
  grid = Grid(3);
  

  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.print("LCD init\n");
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Welcome! Go to:");
  // Move to bottom row
  lcd.setCursor(0,1);
  lcd.print("t.ly/h7NLS");
  Serial.print("Complete!\n");
  //delay(5000);

  // attempt to connect to WiFi network using provisioning:
  Serial.print("Resetting credentials...\n");
  //provisioner.setFactoryResetCallback(myFactoryResetCallback);
  //provisioner.AP_NAME = "Longjump";
  //provisioner.setupAccessPointAndServer();
  //provisioner.connectToWiFi();

  //Serial.println(ssid);
  /*
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }*/

  Serial.println("You're connected to the network\n");
  Serial.println();

  // You can provide a unique client ID, if not set the library uses Arduino-millis()
  // Each client must have a unique client ID
  // mqttClient.setId("clientId");

  // You can provide a username and password for authentication
  // mqttClient.setUsernamePassword("username", "password");
  //delay(5000);

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  /*
  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }
  */

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
}

bool buttons[9] = {false};

void loop()
{
  //Update the inputs and outputs the state of the grid to the board
  grid.Update();

  // call poll() regularly to allow the library to send MQTT keep alives which
  // avoids being disconnected by the broker
  //mqttClient.poll();

  // to avoid having delays in loop, we'll use the strategy from BlinkWithoutDelay
  // see: File -> Examples -> 02.Digital -> BlinkWithoutDelay for more info
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval)
  {

    //mqttClient.beginMessage(topic);
    //mqttClient.println(count);
    Serial.println(count);
    Serial.println(grid.to_string().c_str());
    //mqttClient.println(grid.to_string().c_str());
    //mqttClient.endMessage();

    // save the last time a message was sent
    previousMillis = currentMillis;
    count++;
  }

  //Change color if piece is over it
  for (int x = 0; x < 3; x++)
  {
    for (int y = 0; y < 3; y++)
    {
      if (buttons[x + y * 3])
      {
        uint32_t color = 0xFF0000;
        grid.SetColor(x, y, color);
      }
      else
      {
        uint32_t color = 0x0000FF;
        grid.SetColor(x, y, color);
      }
    }
  }
}