#include <FastLED.h>
#include <LiquidCrystal.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include <WiFiProvisioner.h>
#include <WiFi.h>
#include "Grid.hpp"

#define NUM_TILES 64

#define NUM_LEDS  256
#define LED_PIN   23

// TODO: Assign correct row pins
#define ROW0      18
#define ROW1      4
#define ROW2      19
#define ROW3      19
#define ROW4      19
#define ROW5      19
#define ROW6      19
#define ROW7      19

// TODO: Assign correct col pins
#define COL0      34
#define COL1      35
#define COL2      32
#define COL3      32
#define COL4      32
#define COL5      32
#define COL6      32
#define COL7      32

#define CONF_BUTTON_PIN 22

#define HALL_SENSITIVITY 200

int tiles[NUM_TILES] = {0};
char serverBoard[NUM_TILES] = {0};
int latestPiece = -1;
bool isHost = 0;
bool isTurn = 0;
bool makingOppMove = 0;
bool gameOver = 0;

const char broker[] = "longjump.ip-dynamic.org";
int        port     = 1883;
const char server_topic[] = "boards/to/server";
const char send_to_guest_topic[] = "boards/from/chess";
int boardID = 0;

WiFiProvisioner::WiFiProvisioner provisioner;
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
// RS, E, D4, D5, D6, D7
LiquidCrystal lcd(33, 25, 26, 27, 14, 13);

//FastLED
CRGB leds[NUM_LEDS];

//Grid object
Grid board(leds, 8);

void setup() {
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);

  // Generate random 6-digit board ID
  randomSeed(analogRead(A0)); 
  boardID = random(100000, 1000000);
  pinMode(ROW0, OUTPUT);
  pinMode(ROW1, OUTPUT);
  pinMode(ROW2, OUTPUT);
  pinMode(ROW3, OUTPUT);
  pinMode(ROW4, OUTPUT);
  pinMode(ROW5, OUTPUT);
  pinMode(ROW6, OUTPUT);
  pinMode(ROW6, OUTPUT);

  pinMode(COL0, INPUT);
  pinMode(COL1, INPUT);
  pinMode(COL2, INPUT);
  pinMode(COL3, INPUT);
  pinMode(COL4, INPUT);
  pinMode(COL5, INPUT);
  pinMode(COL6, INPUT);
  pinMode(COL7, INPUT);

  pinMode(CONF_BUTTON_PIN, INPUT);

  FastLED.clear();
  FastLED.show();

  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

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

//Tests grid leds
void TestGrid() {
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(255, i/2, i);
  }
  FastLED.show();
}

void sendToServer() {
      // Publish the board current board state to the server
      StaticJsonDocument<256> doc;

      doc["playerID"] = boardID;

      JsonArray boardState = doc.createNestedArray("boardState");
      for (int i = 0; i < 64; i++) {
        boardState.add(board.GetCurrSensorsAt(i));
      }

      doc["confirmButton"] = digitalRead(CONF_BUTTON_PIN);

      char jsonBuffer[256];
      size_t len = serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
      mqttClient.beginMessage(send_to_guest_topic);
      mqttClient.write((const uint8_t*)jsonBuffer, len);
      mqttClient.endMessage();
}

/*
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
*/

void handleStartGame(const String &payload) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print("JSON deserialization failed: ");
        Serial.println(error.c_str());
        return;
    }

    const char* command = doc["command"];
    const char* message = doc["message"];

    // booleans determine the game being played
    bool isNotChess = (!message);
    bool isNotTicTacToe = (!command);

    if (!isNotChess) {
        if (strcmp(message, "chess_start_host") == 0) {
            board.UpdateLeds(payload);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Chess started!");
            lcd.setCursor(0, 1);
            lcd.print("Updated board!");
        }
    }
    else if (!isNotTicTacToe) {
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
        }
    }
    else {
        Serial.print("Unknown payload: ");
        if (!isNotChess) {
            Serial.println(message);
        }
        else if (!isNotTicTacToe) {
            Serial.println(command);
        }
    }
}

void handleMove(const String &payload) {
    board.UpdateLeds(payload);

    // Next step is to move the opponent's pieces
    makingOppMove = 1;

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Opp moved!");
    lcd.setCursor(0, 1);
    lcd.print("Update board!");
}

void handleWin(const String &payload) {
    board.UpdateLeds(payload);
    gameOver = 1;

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Game over!");
    lcd.setCursor(0, 1);
    lcd.print("You win! :)");
}

void handleLose(const String &payload) {
    board.UpdateLeds(payload);
    gameOver = 1;

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Game over!");
    lcd.setCursor(0, 1);
    lcd.print("You lose. :(");
}

void handleDraw(const String &payload) {
    board.UpdateLeds(payload);
    gameOver = 1;

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
          } else if (payload.indexOf("start")) {
          } else {
              Serial.println("Unknown command in board message.");
              board.UpdateLeds(payload);
          }
      } else {
          Serial.println("Unhandled topic.");
      }
  }

  // Lights up board in predetermined pattern
  // TestGrid();
  
  // Reads hall effect sensors.
  board.UpdateSensors();
  
  // Sends sensor readings to the server if a change is detected.
  if (board.PiecesChanged() || digitalRead(CONF_BUTTON_PIN) == 1)
  {
    board.SaveReadings();
    sendToServer();
  }

  /*
  if (isTurn) {
      // Update LEDs during player turn
      for (int i = 0; i < NUM_LEDS; i++) {
        if (!gameOver) {
          // Valid empty space
          if (tiles[i] < HALL_SENSITIVITY) {
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
  } else if (makingOppMove) {
      
      // If there is a piece on the board, in a place there isn't supposed to be one,
      // then the player is still making the opponent's move.
      makingOppMove = 0;
      for(int i = 0; i < NUM_LEDS; i++) {
        if (tiles[i] < HALL_SENSITIVITY && (serverBoard[i] != '0' || serverBoard[i] != 'P') {
            makingOppMove = 1;
        }
      }

      // Once the opponent's move has been made, your turn begins!
      if (!makingOppMove) {isTurn = 1;}
  }
  */

  delay(100);
}
