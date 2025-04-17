#include <FastLED.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include <WiFiProvisioner.h>
#include <WiFi.h>
#include <Preferences.h>
#include "Grid.hpp"

#define NUM_TILES 64
#define NUM_TILES_ttt 9
#define NUM_LEDS  256
#define NUM_LEDS_ttt  9

// For OG board
//#define LED_PIN   23

// TODO: Assign correct row pins
//#define ROW0      18
//#define ROW1      4
//#define ROW2      19


/*
#define ROW3      19
#define ROW4      19
#define ROW5      19
#define ROW6      19
#define ROW7      19
*/

// TODO: Assign correct col pins
//#define COL0      34
//#define COL1      35
//#define COL2      32
/*
#define COL3      32
#define COL4      32
#define COL5      32
#define COL6      32
#define COL7      32
*/

//#define CONF_BUTTON_PIN 22


// For advanced board
#define LED_PIN   15

// TODO: Assign correct row pins
#define ROW0      13
#define ROW1      23
#define ROW2      16
/*
#define ROW3      27
#define ROW4      27
#define ROW5      27
#define ROW6      27
#define ROW7      27
*/

//33, 25, 26, 27, 14, 13

// TODO: Assign correct col pins
#define COL0      36
#define COL1      39
#define COL2      34
#define COL3      35
#define COL4      32
#define COL5      33

#define CONF_BUTTON_PIN 14

#define HALL_SENSITIVITY 200

int tiles[NUM_TILES] = {0};
int tiles_ttt[NUM_TILES_ttt] = {0};
char serverBoard[NUM_TILES] = {0};
char serverBoard_ttt[NUM_TILES_ttt] = {0};
int latestPiece = -1;
bool isHost = 0;
bool isTurn = 0;
bool makingOppMove = 0;
bool gameOver = 0;
bool isChess = 0;
bool isTicTacToe = 0;
bool connectionLost = 0;
bool gameOverChess = 0;
bool isWinner = 0;

bool gameStart = 0;
int sensorVals[8][8] = {0};
int rowPins_main[8] = {13, 23, 16, 17, 5, 18, 19, 27}; //{13, 23, 16, 17, 5, 18, 19, 27};
int colPins_main[8] = {36, 39, 34, 35, 32, 33, 33, 33}; //{36, 39, 34, 35, 32, 33, 33, 33};

const char broker[] = "longjump.ip-dynamic.org";
int        port     = 1883;
const char server_topic[] = "boards/to/server";
const char send_to_chess_topic[] = "boards/to/chess";
const char send_to_guest_topic[] = "server/from/board";
int boardID = 0;

// Rainbow noise variables
CRGBPalette16 myPalette = RainbowColors_p;
uint16_t noiseX, noiseY;
uint16_t offsetX, offsetY;

Preferences preferences;
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
// RS, E, D4, D5, D6, D7
LiquidCrystal_I2C lcd(0x27,20,4);

//FastLED
CRGB leds[NUM_LEDS];
CRGB leds_ttt[NUM_LEDS_ttt];

//Grid object
Grid board(leds, 8);

DEFINE_GRADIENT_PALETTE(redPalette_gp) {
  0,   32, 0, 0,      // Dark red
  128, 128, 0, 0,     // Medium red
  255, 255, 0, 0      // Bright red
};

DEFINE_GRADIENT_PALETTE(greenPalette_gp) {
  0,   0, 32, 0,      // Dark green
  128, 0, 128, 0,     // Medium green
  255, 0, 255, 0      // Bright green
};

CRGBPalette16 redPalette = redPalette_gp;
CRGBPalette16 greenPalette = greenPalette_gp;

int getLedIndex(int x, int y) {
  if (y % 2 == 0) {
    return (y * 16) + x;
  } else {
    return (y * 16) + (15 - x);
  }
}

uint8_t ease(uint8_t value) {
  float v = value / 255.0;
  return (uint8_t)(255 * (3 * v * v - 2 * v * v * v));
}

void printTwoLines(String text, int cols = 16) {
  Serial.print("Writing message to LCD: ");
  Serial.println(text);
  lcd.clear();

  if (text.length() > cols * 2) {
    Serial.println("Error: String exceeds maximum length (32 chars).");
    return;
  }

  int splitIndex = -1;

  if (text.length() <= cols) {
    // Fits in one line
    lcd.setCursor(0, 0);
    lcd.print(text);
    return;
  }

  // Find a good place to split (last space before or at position cols)
  for (int i = cols; i >= 0; i--) {
    if (text.charAt(i) == ' ') {
      splitIndex = i;
      break;
    }
  }

  if (splitIndex == -1) {
    // No space found; force split at cols
    splitIndex = cols;
  }

  String line1 = text.substring(0, splitIndex);
  String line2 = text.substring(splitIndex);
  line2.trim(); // Remove leading spaces

  // Ensure line2 doesn't exceed 16 chars
  if (line2.length() > cols) {
    line2 = line2.substring(0, cols);
  }

  lcd.setCursor(0, 0);
  lcd.print(line1);

  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// Connect using stored credentials
bool connectToWiFi() {
  preferences.begin("wifi-provision", true);
  String savedSSID = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");
  preferences.end();

  if (savedSSID.isEmpty()) {
    Serial.println("No saved Wi-Fi credentials found.");
    return false;
  }

  Serial.printf("Connecting to saved Wi-Fi: %s\n", savedSSID.c_str());
  if (savedPassword.isEmpty()) {
    WiFi.begin(savedSSID.c_str());
  } else {
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
  }

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > 100000 || digitalRead(CONF_BUTTON_PIN) == 1) {
      Serial.println("Failed to connect to saved Wi-Fi or Credentials reset");
      return false;
    }
    delay(500);
  }

  Serial.printf("Successfully connected to %s\n", savedSSID.c_str());
  return true;
}

void setup() {
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(100);

  pinMode(ROW0, OUTPUT);
  pinMode(ROW1, OUTPUT);
  pinMode(ROW2, OUTPUT);

  pinMode(26, OUTPUT);
  pinMode(25, OUTPUT);

  pinMode(17, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(27, OUTPUT);

  pinMode(COL0, INPUT);
  pinMode(COL1, INPUT);
  pinMode(COL2, INPUT);
  pinMode(COL3, INPUT);
  pinMode(COL4, INPUT);
  pinMode(COL5, INPUT);

  digitalWrite(ROW0, HIGH);
  digitalWrite(ROW1, HIGH);   
  digitalWrite(ROW2, HIGH); 

  random16_add_entropy(analogRead(36));
  noiseX = random16();
  noiseY = random16();
  offsetX = random8(3, 8);
  offsetY = random8(3, 8);

  pinMode(CONF_BUTTON_PIN, INPUT);

  FastLED.clear();
  FastLED.show();

  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  //analogReadResolution(12);

  // Generate random 6-digit board ID
  randomSeed(analogRead(36)); 
  boardID = random(100000, 1000000);

  lcd.init();
  lcd.backlight();
  lcd.print("Go to:t.ly/h7NLS");
  lcd.setCursor(0, 1);
  lcd.print("ID: ");
  lcd.print(boardID);

  // Set up provisioner
  char boardSSID[32];
  snprintf(boardSSID, sizeof(boardSSID), "LongJump-%d", boardID);

  WiFiProvisioner provisioner(
      {boardSSID, "LongJump WiFi Connection", "dodgerblue", "<default_svg>", 
      "LongJump", "Device Setup", "Follow the steps to provision your device", 
      "All rights reserved Â© WiFiProvisioner", "Your device is now provisioned.", 
      "This process cannot be undone.", "Device Key", 4, false, true});

  // Configure to hide additional fields
  provisioner.getConfig().SHOW_INPUT_FIELD = false; // No additional input field
  provisioner.getConfig().SHOW_RESET_FIELD = false; // No reset field

  // Set callbacks
  provisioner
      .onFactoryReset([]() {
        preferences.begin("wifi-provision", false);
        Serial.println("Factory reset triggered! Clearing stored preferences...");
        preferences.clear(); // Clear all stored credentials and API key
        preferences.end();
      })
      .onSuccess([](const char *ssid, const char *password, const char *input) {
        Serial.printf("Provisioning successful! SSID: %s\n", ssid);
        preferences.begin("wifi-provision", false);
        // Store the credentials and API key in preferences
        preferences.putString("ssid", String(ssid));
        if (password) {
          preferences.putString("password", String(password));
          Serial.printf("Password: %s\n", password);
        }
        preferences.end();
        Serial.println("Credentials stored for future use.");
      });

  Serial.println("Connecting to WiFi...");
  if (!connectToWiFi()) {
    Serial.println("No stored credentials found...Restarting provisioning process");
    // Start the provisioning process
    provisioner.startProvisioning();
  }

  //Serial.println("------------- PRINTING MAC ADDRESS --------------");
  //Serial.println(WiFi.macAddress());

  lcd.clear();
  lcd.print("WiFi connected!");
  lcd.setCursor(0, 1);
  lcd.print("ID: ");
  lcd.print(boardID);

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

  // DEBUG
  //isTicTacToe = 1;
  //isHost = 1;
  //isTurn = 1;
}

void sendToServer() {
    Serial.println("CALLED: sendToServer");

    // Read current board into 2D array
    int original[8][8];
    for (int i = 0; i < 64; i++) {
        int row = i / 8;
        int col = i % 8;
        original[row][col] = board.GetCurrSensorsAt(i);
    }

    // Rotate 90 degrees CCW
    int rotatedCCW[8][8];
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            rotatedCCW[r][c] = original[c][7 - r];
        }
    }

    //TODO: If guest, rotate 90 degrees CW?
    /*
    if (isHost) {
      // Rotate 90 degrees CCW
      int rotatedCCW[8][8];
      for (int r = 0; r < 8; r++) {
          for (int c = 0; c < 8; c++) {
              rotatedCCW[r][c] = original[c][7 - r];
          }
      }
    } else {
      // Rotate 90 degrees CW
    }
    */

    // Send board
    StaticJsonDocument<256> doc;
    doc["playerID"] = boardID;
    JsonArray boardState = doc.createNestedArray("boardState");

    // Flatten in row-major order
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            boardState.add(rotatedCCW[r][c]);
        }
    }

    doc["confirmButton"] = digitalRead(CONF_BUTTON_PIN);

    // Debug: Print out the rotated 2D array
    Serial.println("DEBUG: Rotated (90 ccw) board state about to be sent:");
    for (int row = 0; row < 8; row++) {
      Serial.print("  [ ");
      for (int col = 0; col < 8; col++) {
        if (col > 0) {
          Serial.print(", ");
        }
        Serial.print(rotatedCCW[row][col]);
      }
      Serial.println(" ]");
    }

    // Serialize and send the JSON
    char jsonBuffer[256];
    size_t len = serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));
    mqttClient.beginMessage(send_to_chess_topic);
    mqttClient.write((const uint8_t*)jsonBuffer, len);
    mqttClient.endMessage();
}

// Call upon confirm button press
void sendMoveToServer() {
      // End turn
      isTurn = 0;

      // Publish the board current board state to the server
      StaticJsonDocument<256> doc;
      doc["playerID"] = boardID;
      JsonArray boardState = doc.createNestedArray("BoardState");

      for (int i = 0; i < NUM_LEDS_ttt; i++) {
        if (leds_ttt[i] == CRGB::Red || leds_ttt[i] == CRGB::Blue) {
            boardState.add(1);
        } else {
            boardState.add(0);
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
    gameStart = 1;
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
    isChess = (message);
    isTicTacToe = (command);
    Serial.print("isChess: ");
    Serial.println(isChess);
    Serial.print("isTicTacToe: ");
    Serial.println(isTicTacToe);

    if (isChess) {
        if (strcmp(message, "chess_start_host") == 0) {
            //Serial.println("---CHESS START CMD RECEIVED---");
            //Serial.print("---gameStart: ");
            //Serial.println(gameStart);
            isHost = 1;
            FastLED.clear();
            board.UpdateLeds(payload);
            FastLED.show();
            //Serial.println("---CHECKERBOARD SHOULD APPEAR---");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Chess started!");
            lcd.setCursor(0, 1);
            lcd.print("Your turn!");
        } else if (strcmp(message, "chess_start_guest") == 0) {
            isHost = 0;
            FastLED.clear();
            board.UpdateLeds(payload);
            FastLED.show();
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Chess started!");
            lcd.print("Waiting for host");
        } else {
            while(1) {
              Serial.println("ERROR: Invalid Chess start message received");
            }
        }
    }
    else if (isTicTacToe) {
            digitalWrite(17, HIGH);
            digitalWrite(5, HIGH);
            digitalWrite(18, HIGH);
            digitalWrite(19, HIGH);
            digitalWrite(27, HIGH);

            Serial.println("Before StartTTT");
            board.StartTTT();
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
        if (isChess) {
            Serial.println(message);
        }
        else if (isTicTacToe) {
            Serial.println(command);
        }
    }
}

void handleMove(const String &payload) {
  if(isTicTacToe) {
    board.UpdateLedsTTT(payload);
    
    // Begin turn
    isTurn = 1;

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Your turn!");

  } else if (isChess) {
    board.UpdateLeds(payload);

    // Next step is to move the opponent's pieces
    //makingOppMove = 1;

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Opp moved!");
    lcd.setCursor(0, 1);
    lcd.print("Update board!");
  }
}

void handleWin(const String &payload) {
  if (isTicTacToe) {
    board.UpdateLedsTTT(payload);

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Game over!");
    lcd.setCursor(0, 1);
    lcd.print("You win! :)");

  } else if (isChess) {
    board.UpdateLeds(payload);
    gameOverChess = 1;
    isWinner = 1;

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Game over!");
    lcd.setCursor(0, 1);
    lcd.print("You win! :)");
  }
}

void handleLose(const String &payload) {
  if (isTicTacToe) {
      board.UpdateLedsTTT(payload);
      // Update LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Game over!");
      lcd.setCursor(0, 1);
      lcd.print("You lose. :(");
  } else if (isChess) {
    board.UpdateLeds(payload);
    gameOverChess = 1;
    isWinner = 0;

    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Game over!");
    lcd.setCursor(0, 1);
    lcd.print("You lose. :(");
  }
}

void handleDraw(const String &payload) {
  if (isTicTacToe) {
    board.UpdateLedsTTT(payload);
  } else if (isChess) {
    board.UpdateLeds(payload);
    gameOver = 1;
  }
    // Update LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Game over!");
    lcd.setCursor(0, 1);
    lcd.print("Draw. :|");
}

void loop() {
  // Check wifi connection status and reconnect if disconnected
  if (WiFi.status() == 3) {
    connectionLost = 0;
  } else {
    connectionLost = 1;
  }

  if (gameStart == 0) {
    if (connectionLost == 0) {
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
                  return;
              } else if (payload.indexOf("move") != -1) {
                  Serial.println("Move command received");
                  handleMove(payload);
                  return;
              } else if (payload.indexOf("win") != -1) {
                  Serial.println("Win command received");
                  handleWin(payload);
                  //return;
              } else if (payload.indexOf("lose") != -1) {
                  Serial.println("Lose command received");
                  handleLose(payload);
                  //return;
              } else if (payload.indexOf("draw") != -1) {
                  Serial.println("Draw command received");
                  handleDraw(payload);
                  //return;
              } else if (payload.indexOf("message") != -1)  {
                  // Handle message
                  StaticJsonDocument<256> doc;
                  DeserializationError error = deserializeJson(doc, payload);

                  if (error) {
                      Serial.print("JSON deserialization failed: ");
                      Serial.println(error.c_str());
                      return;
                  }

                  const char* message = doc["message"];
                  Serial.print("Received message from server: ");
                  Serial.println(String(message));
                  // Print message contents to LCD
                  printTwoLines(String(message));

                  Serial.println("Chess message received.");
                  board.UpdateLeds(payload);
              } else {
                  Serial.println("Unknown command in board message.");
                  board.UpdateLeds(payload);
              }
          } else {
              Serial.println("Unhandled topic.");
          }
      }
    }

    // Read sensor values
    for (int i = 0; i < 8; i++) {
      digitalWrite(rowPins_main[i], LOW);

      for (int j = 0; j < 8; j++) {
        if (j <= 4) {
          sensorVals[i][j] = analogRead(colPins_main[j]);
        } else {
          if (j == 5) {
            digitalWrite(26, LOW);
            digitalWrite(25, LOW);
          } else if (j == 6) {
            digitalWrite(26, HIGH);
            digitalWrite(25, LOW);
          } else {
            digitalWrite(26, LOW);
            digitalWrite(25, HIGH);
          }
          delayMicroseconds(50);
          sensorVals[i][j] = analogRead(33);
        }
      }

      digitalWrite(rowPins_main[i], HIGH);
    }

    //NEW
    bool allCorrect = true;
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        bool piecePresent = sensorVals[i][j] < 100;
        bool shouldBePresent = (j <= 1 || j >= 6);  // only outer columns should have pieces
        if (piecePresent != shouldBePresent) {
          allCorrect = false;
          break;
        }
      }
      if (!allCorrect) break;
    }

    for (int y = 0; y < 16; y++) {
      for (int x = 0; x < 16; x++) {
        int sensorRow = y / 2;
        int sensorCol = x / 2;

        bool piecePresent = sensorVals[sensorRow][sensorCol] < 100;
        bool isChessBlackSquare = (sensorRow + sensorCol) % 2 == 1;

        if (piecePresent) {
          leds[getLedIndex(x, y)] = allCorrect ? CRGB::Green : CRGB::White;
        } else if (isChessBlackSquare) {
          leds[getLedIndex(x, y)] = CRGB::Black;
        } else {
          uint8_t noiseValue = inoise8(noiseX + x * 20, noiseY + y * 20);
          uint8_t easedNoise = ease(noiseValue);
          CRGB color = ColorFromPalette(myPalette, easedNoise);
          color.nscale8(150);
          leds[getLedIndex(x, y)] = color;
        }
      }
    }
    FastLED.show();


    // Animate noise movement
    noiseX += offsetX + random8(1, 3);
    noiseY += offsetY + random8(1, 3);

    if (random8() < 15) {
      offsetX = random8(3, 8);
      offsetY = random8(3, 8);
    }

    delay(50);

  } else {
    if (connectionLost == 0) {
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
              } else if ((payload.indexOf("move") != -1) && isTicTacToe == 1) {
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
              } else if (payload.indexOf("message") != -1) {
                  // Handle message
                  StaticJsonDocument<256> doc;
                  DeserializationError error = deserializeJson(doc, payload);

                  if (error) {
                      Serial.print("JSON deserialization failed: ");
                      Serial.println(error.c_str());
                      return;
                  }

                  const char* message = doc["message"];
                  Serial.print("Received message from server: ");
                  Serial.println(String(message));
                  // Print message contents to LCD
                  printTwoLines(String(message));

                  Serial.println("Chess message received.");
                  board.UpdateLeds(payload);
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
      
      // Sends sensor readings to the server if a change is detected.
      if(isChess) {
        if (gameOverChess == 0) {
          //Serial.println("Game is CHESS");
          // Reads hall effect sensors.
          board.UpdateSensors();

          if (board.PiecesChanged() || digitalRead(CONF_BUTTON_PIN) == 1) {
            if (digitalRead(CONF_BUTTON_PIN == 1)) {
              Serial.println("Confirm button pressed");
            }
            board.SaveReadings();
            sendToServer();
          }

          FastLED.show();
        } else {
          // Enter gameOver state based on winner
          if (isWinner == 1) {
            // Green pattern
            for (int y = 0; y < 16; y++) {
              for (int x = 0; x < 16; x++) {
                int sensorRow = y / 2;
                int sensorCol = x / 2;

                bool piecePresent = 0;
                bool isChessBlackSquare = (sensorRow + sensorCol) % 2 == 1;

                if (piecePresent) {
                  leds[getLedIndex(x, y)] = CRGB::Red;
                } else if (isChessBlackSquare) {
                  leds[getLedIndex(x, y)] = CRGB::Black;
                } else {
                  uint8_t noiseValue = inoise8(noiseX + x * 6, noiseY + y * 6);
                  //uint8_t easedNoise = ease(noiseValue);
                  CRGB color = ColorFromPalette(greenPalette, noiseValue);
                  //color.nscale8(220);  // optional: tone it down
                  leds[getLedIndex(x, y)] = color;
                }
              }
            }
            FastLED.show();

            // Animate noise movement
            noiseX += offsetX + random8(5, 10);
            noiseY += offsetY + random8(5, 10);

            if (random8() < 15) {
              offsetX = random8(3, 8);
              offsetY = random8(3, 8);
            }

            delay(50);

          } else {
            // Red pattern
            for (int y = 0; y < 16; y++) {
              for (int x = 0; x < 16; x++) {
                int sensorRow = y / 2;
                int sensorCol = x / 2;

                bool piecePresent = 0;
                bool isChessBlackSquare = (sensorRow + sensorCol) % 2 == 1;

                if (piecePresent) {
                  leds[getLedIndex(x, y)] = CRGB::Red;
                } else if (isChessBlackSquare) {
                  leds[getLedIndex(x, y)] = CRGB::Black;
                } else {
                  uint8_t noiseValue = inoise8(noiseX + x * 6, noiseY + y * 6);
                  //uint8_t easedNoise = ease(noiseValue);
                  CRGB color = ColorFromPalette(redPalette, noiseValue);
                  //color.nscale8(220);  // optional: tone it down
                  leds[getLedIndex(x, y)] = color;
                }
              }
            }
            FastLED.show();

            // Animate noise movement
            noiseX += offsetX + random8(5, 10);
            noiseY += offsetY + random8(5, 10);

            if (random8() < 15) {
              offsetX = random8(3, 8);
              offsetY = random8(3, 8);
            }

            delay(50);
          }
        }
      }

      if(isTicTacToe) {
        digitalWrite(ROW0, LOW);
        delay(10);

        tiles_ttt[0] = analogRead(COL0);

        //tiles_ttt[1] = analogRead(COL1);
        tiles_ttt[1] = analogRead(COL1);

        tiles_ttt[2] = analogRead(COL2);
        digitalWrite(ROW0, HIGH);
        delay(10);

        // ROW1
        //digitalWrite(ROW0, HIGH);
        digitalWrite(ROW1, LOW);
        delay(10);
        //digitalWrite(ROW2, HIGH);

        tiles_ttt[5] = analogRead(COL0);

        //tiles_ttt[4] = analogRead(COL1);
        tiles_ttt[4] = analogRead(COL1);

        tiles_ttt[3] = analogRead(COL2);
        digitalWrite(ROW1, HIGH);
        delay(10);

        // ROW2
        //digitalWrite(ROW0, HIGH);
        //digitalWrite(ROW1, HIGH);
        digitalWrite(ROW2, LOW);
        delay(10);
        tiles_ttt[6] = analogRead(COL0);

        //tiles_ttt[7] = analogRead(COL1);
        tiles_ttt[7] = analogRead(COL1);

        tiles_ttt[8] = analogRead(COL2);
        digitalWrite(ROW2, HIGH);
        delay(10);

        for (int i = 0; i < 9; i++) {
          Serial.print(tiles_ttt[i]);
          Serial.print(" ");
        }
        Serial.println();

        if (isTurn) {
            // Update LEDs during player turn
            for (int i = 0; i < NUM_LEDS_ttt; i++) {
              if (!gameOver) {
                // Valid empty space
                Serial.print(tiles_ttt[i]);
                Serial.print(" ");
                if (tiles_ttt[i] < HALL_SENSITIVITY) {
                    // Set LED to Red for host and blue for guest
                    if (isHost && leds_ttt[i] == CRGB::White){
                      leds_ttt[i] = CRGB::Red;
                    } else if (!isHost && leds_ttt[i] == CRGB::White) {
                      leds_ttt[i] = CRGB::Blue;
                    }
                } else {
                    // Set LED to Black (off) and add to board state
                    leds_ttt[i] = CRGB::White;
                }
              }
            }
            // Show updated LED state
            board.LiveLedsTTT(leds_ttt);
          // Check if confirm button pressed
          if (digitalRead(CONF_BUTTON_PIN) == 1) {
            Serial.println("Confirm button pressed");
            //sendMoveToServer();
          }
        }
      }
        delay(10);
    } else {
      // reconnect to wifi
      Serial.println("WiFi connection lost! Reconnecting....");
      lcd.setCursor(0, 0);
      lcd.print("Connection lost!");
      lcd.setCursor(0, 1);
      lcd.print("Reconnecting....");
      connectToWiFi();

      // reconnect to MQTT server
      Serial.print("Attempting to connect to the MQTT broker: ");
      Serial.println(broker);


      if (!mqttClient.connect(broker, port)) {
        Serial.print("MQTT connection failed! Error code = ");
        Serial.println(mqttClient.connectError());

        while (1);
      }

      Serial.println("You're connected to the MQTT broker!");
      Serial.println(); 

      // Connect to board-specific MQTT topic "board/boardID"
      char boardTopic[32];
      snprintf(boardTopic, sizeof(boardTopic), "board/%d", boardID);

      if (!mqttClient.subscribe(boardTopic)) {
          Serial.println("Failed to subscribe to topic");
      } else {
        Serial.print("Subscribed to topic: ");
        Serial.println(boardTopic);
      }

      printTwoLines("WiFi reconnected!");
      delay(500);
    }
  }
}
