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

#define CONF_BUTTON_PIN 22

int tiles[9] = {0};
int latestPiece = -1;
int serverBoard[9] = {0};
bool isHost = 0;
bool isTurn = 0;

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

  pinMode(CONF_BUTTON_PIN, INPUT);

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
  //provisioner.setupAccessPointAndServer();
  provisioner.connectToWiFi();

  lcd.clear();
  lcd.print("WiFi connected!");
  lcd.setCursor(0, 1);
  lcd.print("Waiting to start");

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

// Call upon confirm button press
void sendMoveToServer() {
      // End turn
      isTurn = 0;

      // Publish the board current board state to the server
      StaticJsonDocument<256> doc;
      doc["playerID"] = boardID;
      JsonArray boardState = doc.createNestedArray("boardState");

      for (int i = 0; i < NUM_LEDS; i++) {
        if (tiles[i] < 200) {
            // Add to board state
            boardState.add(1);
        } else {
            // Set LED to Black (off) and add to board state
            strip.setPixelColor(i, strip.Color(0, 0, 0));
            boardState.add(0);  // Example: 0 for inactive
        }
      }

      char jsonBuffer[256];
      size_t len = serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
      mqttClient.beginMessage(send_to_guest_topic);
      mqttClient.write((const uint8_t*)jsonBuffer, len);
      mqttClient.endMessage();

      // Update LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Waiting for");
      lcd.setCursor(0, 1);
      lcd.print("opponent...");
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
        isTurn = 1;
        isHost = 1;
        lcd.setCursor(0, 0);
        lcd.print("Game start!");
        lcd.setCursor(0, 1);
        lcd.print("Your turn!");
    } else if (strcmp(command, "start_guest") == 0) {
        Serial.println("Game Start: You are the guest.");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Waiting for");
        lcd.setCursor(0, 1);
        lcd.print("opponent...");

        isHost = 0;
    } else {
        Serial.print("Unknown command: ");
        Serial.println(command);
    }
}


void handleMove(const String &payload) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print("JSON deserialization failed: ");
        Serial.println(error.c_str());
        return;
    }

    // Expecting payload to contain the board state as an array
    JsonArray boardState = doc["BoardState"];
    if (!boardState) {
        Serial.println("Board state not found in payload.");
        return;
    }

    // Update LEDs based on received board state
    for (int i = 0; i < NUM_LEDS; i++) {
        if (boardState[i] == 1) {
          // Red host piece placed
          strip.setPixelColor(i, strip.Color(255, 0, 0));
          // Update serverBoard
          serverBoard[i] = boardState[i];
        } else if (boardState[i] == 2) {
          // Blue guest piece placed
          strip.setPixelColor(i, strip.Color(0, 0, 255));
          // Update serverBoard
          serverBoard[i] = boardState[i];
        } else {
          strip.setPixelColor(i, strip.Color(0, 0, 0));
        }
    }
    strip.show();

    // Begin turn
    isTurn = 1;

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Your turn!");
}

void handleWin(const String &payload) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print("JSON deserialization failed: ");
        Serial.println(error.c_str());
        return;
    }

    // Expecting payload to contain the board state as an array
    JsonArray boardState = doc["BoardState"];
    if (!boardState) {
        Serial.println("Board state not found in payload.");
        return;
    }

    // Update LEDs based on received board state
    for (int i = 0; i < NUM_LEDS; i++) {
        if (boardState[i] == 1) {
          // Red host piece placed
          strip.setPixelColor(i, strip.Color(255, 0, 0));
          // Update serverBoard
          serverBoard[i] = boardState[i];
        } else if (boardState[i] == 2) {
          // Blue guest piece placed
          strip.setPixelColor(i, strip.Color(0, 0, 255));
          // Update serverBoard
          serverBoard[i] = boardState[i];
        } else {
          strip.setPixelColor(i, strip.Color(0, 0, 0));
        }
    }
    strip.show();

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Game over!");
    lcd.setCursor(0, 1);
    lcd.print("You win! :)");
}

void handleLose(const String &payload) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print("JSON deserialization failed: ");
        Serial.println(error.c_str());
        return;
    }

    // Expecting payload to contain the board state as an array
    JsonArray boardState = doc["BoardState"];
    if (!boardState) {
        Serial.println("Board state not found in payload.");
        return;
    }

    // Update LEDs based on received board state
    for (int i = 0; i < NUM_LEDS; i++) {
        if (boardState[i] == 1) {
          // Red host piece placed
          strip.setPixelColor(i, strip.Color(255, 0, 0));
          // Update serverBoard
          serverBoard[i] = boardState[i];
        } else if (boardState[i] == 2) {
          // Blue guest piece placed
          strip.setPixelColor(i, strip.Color(0, 0, 255));
          // Update serverBoard
          serverBoard[i] = boardState[i];
        } else {
          strip.setPixelColor(i, strip.Color(0, 0, 0));
        }
    }
    strip.show();

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Game over!");
    lcd.setCursor(0, 1);
    lcd.print("You lose. :(");
}

void handleDraw(const String &payload) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print("JSON deserialization failed: ");
        Serial.println(error.c_str());
        return;
    }

    // Expecting payload to contain the board state as an array
    JsonArray boardState = doc["BoardState"];
    if (!boardState) {
        Serial.println("Board state not found in payload.");
        return;
    }

    // Update LEDs based on received board state
    for (int i = 0; i < NUM_LEDS; i++) {
        if (boardState[i] == 1) {
          // Red host piece placed
          strip.setPixelColor(i, strip.Color(255, 0, 0));
          // Update serverBoard
          serverBoard[i] = boardState[i];
        } else if (boardState[i] == 2) {
          // Blue guest piece placed
          strip.setPixelColor(i, strip.Color(0, 0, 255));
          // Update serverBoard
          serverBoard[i] = boardState[i];
        } else {
          strip.setPixelColor(i, strip.Color(0, 0, 0));
        }
    }
    strip.show();

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Game over!");
    lcd.setCursor(0, 1);
    lcd.print("Draw. :|");
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
          } else if (payload.indexOf("move") != -1) {
              Serial.println("Move command received");
              handleMove(payload);
          } else if (payload.indexOf("win") != -1) {
              Serial.println("Win command received");
              handleWin(payload);
          } else if (payload.indexOf("lose") != -1) {
              Serial.println("Lose command received");
              handleLose(payload);
          } else if (payload.indexOf("draw") != -1) {
              Serial.println("Draw command received");
              handleDraw(payload);
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

  if (isTurn) {
      // Update LEDs during player turn
      for (int i = 0; i < NUM_LEDS; i++) {
        if (serverBoard[i] == 0) {
          // Valid empty space
          if (tiles[i] < 200) {
              // Set LED to Red for host and blue for guest
              if (isHost){
                strip.setPixelColor(i, strip.Color(255, 0, 0)); 
              } else {
                strip.setPixelColor(i, strip.Color(0, 0, 255));
              }
          } else {
              // Set LED to Black (off) and add to board state
              strip.setPixelColor(i, strip.Color(0, 0, 0));
          }
        }
      }
      // Show updated LED state
      strip.show();
    // Check if confirm button pressed
    if (digitalRead(CONF_BUTTON_PIN) == 1) {
      Serial.println("Confirm button pressed");
      sendMoveToServer();
    }
  }

  delay(100);
}