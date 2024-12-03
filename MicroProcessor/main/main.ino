#include <Adafruit_NeoPixel.h>
#include <WiFiProvisioner.h>
#include <LiquidCrystal.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>


#define NUM_LEDS  9
#define LED_PIN   23

#define ROW0      18
#define ROW1      4
#define ROW2      19

#define COL0      34
#define COL1      35
#define COL2      32

int tiles[9] = {0};
int latestPiece = -1;
char serverBoard[9] = {0};
bool isHost = 0;

const char broker[] = "longjump.ip-dynamic.org";
int        port     = 1883;
const char server_topic[] = "boards/to/server";
const char send_to_guest_topic[] = "server/from/board";
int boardID = 0;

WiFiProvisioner::WiFiProvisioner provisioner;
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
// RS, E, D4, D5, D6, D7
LiquidCrystal lcd(33, 25, 26, 27, 14, 13);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // Generate random 6-digit board ID
  randomSeed(analogRead(A0)); 
  boardID = random(100000, 1000000);
  pinMode(ROW0, OUTPUT);
  pinMode(ROW1, OUTPUT);
  pinMode(ROW2, OUTPUT);

  pinMode(COL0, INPUT);
  pinMode(COL1, INPUT);
  pinMode(COL2, INPUT);

  pinMode(22, INPUT_PULLUP);

  strip.begin();
  strip.setBrightness(100);

  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  strip.clear();

  analogReadResolution(12);

  Serial.print("LCD init\n");
  lcd.begin(16, 2);
  lcd.print("Go to:t.ly/h7NLS");
  lcd.setCursor(0, 1);
  lcd.print("ID: ");
  lcd.print(boardID);

  Serial.print("Complete!\n");

  // WiFi initialization through provisioning
  Serial.print("Resetting credentials...\n");
  //provisioner.setFactoryResetCallback(myFactoryResetCallback);
  char boardSSID[32];
  snprintf(boardSSID, sizeof(boardSSID), "Longjump-%d", boardID);
  provisioner.AP_NAME = boardSSID;
  provisioner.setupAccessPointAndServer();
  provisioner.connectToWiFi();

  Serial.println("You're connected to the network\n");
  Serial.println();

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);


  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  Serial.println("Sending board ID to boards/to/server");
  Serial.println(boardID);
  mqttClient.beginMessage(server_topic);
  mqttClient.println(boardID);
  mqttClient.endMessage();

  // Connect to board-specific MQTT topic "board/boardID"
  char boardTopic[32];
  snprintf(boardTopic, sizeof(boardTopic), "board/%d", boardID);

  if (!mqttClient.subscribe(boardTopic)) {
      Serial.println("Failed to subscribe to topic");
  } else {
    Serial.print("Subscribed to topic: ");
    Serial.println(boardTopic);
  }
}

void sendMoveToServer(int position) {
  // TODO
}

void handleStartGame(const String &payload) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print("JSON deserialization failed: ");
        Serial.println(error.c_str());
        return;
    }

    const char* command = doc["command"];

    if (!command) {
        Serial.println("Command not found in payload");
        return;
    }

    if (strcmp(command, "start_host") == 0) {
        Serial.println("Game Start: You are the host.");
        lcd.clear();
        isHost = 1;
        lcd.setCursor(0, 0);
        lcd.print("Your turn!");
    } else if (strcmp(command, "start_guest") == 0) {
        Serial.println("Game Start: You are the guest.");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Waiting for");
        lcd.setCursor(0, 1);
        lcd.print("host...");

        isHost = 0;
    } else {
        Serial.print("Unknown command: ");
        Serial.println(command);
    }
}

/*
void handleMove(const String &payload) {
    // Unpack received data from host board
    // Display to board LEDs
    // YOUR CODE HERE
}
*/

void handleMove(const String &payload) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print("JSON deserialization failed: ");
        Serial.println(error.c_str());
        return;
    }

    // Expecting payload to contain the board state as an array
    JsonArray boardState = doc["boardState"];
    if (!boardState) {
        Serial.println("Board state not found in payload.");
        return;
    }

    // Update LEDs based on received board state
    for (int i = 0; i < NUM_LEDS; i++) {
        if (boardState[i] == 1) {
            strip.setPixelColor(i, strip.Color(255, 0, 0));
        } else if (boardState[i] == 2) {
            strip.setPixelColor(i, strip.Color(0, 255, 0));
        } else {
            strip.setPixelColor(i, strip.Color(0, 0, 0));
        }
    }
    strip.show();
}

void loop() {
  int messageSize = mqttClient.parseMessage();
  if (messageSize) {
      //Serial.println("Message received");
      String topic = mqttClient.messageTopic();
      String payload = "";

      // Read the message payload
      while (mqttClient.available()) {
          payload += (char)mqttClient.read();
      }

      Serial.print("Received a message with topic: ");
      Serial.println(topic);
      Serial.print("Payload: ");
      Serial.println(payload);

      // Delegate handling based on topic
      if (topic.startsWith("board/")) {
          //Serial.println("Command received");
          if (payload.indexOf("start") != -1) {
              Serial.println("Start command received");
              handleStartGame(payload);
          } else if (payload.indexOf("boardState") != -1) {
              Serial.println("Move command received");
              handleMove(payload);
          } else {
              Serial.println("Unknown command in board message.");
          }
      } else {
          Serial.println("Unhandled topic.");
      }
  }

  // ROW 0
  digitalWrite(ROW0, LOW);
  digitalWrite(ROW1, HIGH);
  digitalWrite(ROW2, HIGH);

  tiles[0] = analogRead(COL0);
  tiles[1] = analogRead(COL1);
  tiles[2] = analogRead(COL2);
  delay(5);

  // ROW1
  digitalWrite(ROW0, HIGH);
  digitalWrite(ROW1, LOW);
  digitalWrite(ROW2, HIGH);

  tiles[5] = analogRead(COL0);
  tiles[4] = analogRead(COL1);
  tiles[3] = analogRead(COL2);
  delay(5);

  // ROW2
  digitalWrite(ROW0, HIGH);
  digitalWrite(ROW1, HIGH);
  digitalWrite(ROW2, LOW);

  tiles[6] = analogRead(COL0);
  tiles[7] = analogRead(COL1);
  tiles[8] = analogRead(COL2);
/*
  if (isHost) {
    // Send current board state to server on send_to_guest_topic[]
    // YOUR CODE HERE

    // Update LEDs based on analog values
    for (int i = 0; i < NUM_LEDS; i++) {
      if (tiles[i] < 200) {
        // Set LED to Red
        strip.setPixelColor(i, strip.Color(255, 0, 0)); 
      }
      else {
        // Set LED to Black (off)
        strip.setPixelColor(i, strip.Color(0, 0, 0));
      }
    }
    strip.show();  // Update the LEDs with new colors
  }
  */

  if (isHost) {
      // Prepare the board state to send
      StaticJsonDocument<256> doc;
      doc["playerID"] = boardID;  // Add the unique host ID
      JsonArray boardState = doc.createNestedArray("boardState");

      // Update host LEDs and prepare the state for publishing
      for (int i = 0; i < NUM_LEDS; i++) {
          if (tiles[i] < 200) {
              // Set LED to Red and add to board state
              strip.setPixelColor(i, strip.Color(255, 0, 0)); 
              boardState.add(1);  // Example: 1 for active
          } else {
              // Set LED to Black (off) and add to board state
              strip.setPixelColor(i, strip.Color(0, 0, 0));
              boardState.add(0);  // Example: 0 for inactive
          }
      }

      // Show the updated LED state on the host
      strip.show();

      // Publish the board state to the server
      char jsonBuffer[256];
      size_t len = serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
      mqttClient.beginMessage(send_to_guest_topic);
      mqttClient.write((const uint8_t*)jsonBuffer, len);
      mqttClient.endMessage();

  }

  delay(100);
}