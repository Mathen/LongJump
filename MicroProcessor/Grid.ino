#include "Grid.hpp"

const int rowPins[8] = {13, 23, 16, 17, 5, 18, 19, 27,};
const int colPins[8] = {36, 39, 34, 35, 32, 33, 33, 33};
const int MUX_SEL_0_PIN = 26;
const int MUX_SEL_1_PIN = 25;
const int hallSense = 200;

Grid::Grid() : dimX(0), dimY(0)
{
  leds = nullptr;
}
Grid::Grid(CRGB* ledArray, unsigned int dimention) : dimX(dimention), dimY(dimention)
{
  leds = ledArray;
}
Grid::Grid(CRGB* ledArray, unsigned int dimentionX, unsigned int dimentionY) : dimX(dimentionX), dimY(dimentionY)
{
  leds = ledArray;
}

int getLedIndexGridClass(int row, int col) {
    if (row % 2 == 0) {
        return row * 16 + col;
    } else {
        return row * 16 + (15 - col);
    }
}

void Grid::UpdateLeds(const String &payload)
{
    Serial.println("Updating LEDs with received message");
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print("JSON deserialization failed: ");
        Serial.println(error.c_str());
        return;
    }

    JsonArray boardState = doc["ledArray"];
    if (!boardState) {
        Serial.println("Board state not found in payload.");
        return;
    }

    // Read incoming board into a 2D array
    char oldBoard[8][8];
    for (int i = 0; i < 64; i++) {
        int row = i / 8;
        int col = i % 8;
        const char* cStr = boardState[i];
        if (cStr) {
            oldBoard[row][col] = cStr[0];
        } else {
            oldBoard[row][col] = '0';
        }
    }

    // Rotate board 90 degrees CCW
    char rotated[8][8];
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            rotated[r][c] = oldBoard[c][7 - r];
        }
    }

    // Mirror the rotated array across the HORIZONTAL axis.
    char mirrored[8][8];
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            mirrored[r][c] = rotated[7 - r][c];
        }
    }

    /*
    // --- Print the final 2D array for debugging ---
    Serial.println("FINAL 2D ARRAY (after rotation CCW and horizontal mirror):");
    for (int r = 0; r < 8; r++) {
        Serial.print("Row ");
        Serial.print(r);
        Serial.print(": [ ");
        for (int c = 0; c < 8; c++) {
            Serial.print(mirrored[r][c]);
            if (c < 7) {
                Serial.print(", ");
            }
        }
        Serial.println(" ]");
    }
    */

    // Flatten mirrored board into a 1D array of length 64 to update LEDs
    char finalFlattened[64];
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int idx = r * 8 + c;
            finalFlattened[idx] = mirrored[r][c];
        }
    }

    // q values represent the 4 LED quadrants of a tile
    int q1 = 0;
    int q2 = 1;
    int q3 = 30;
    int q4 = 31;

    // Use finalFlattened to set LEDs
    for (int i = 0; i < 64; i++) {
        char c = finalFlattened[i];
        CRGB color = CharToColor(c);

        leds[q1] = color;
        leds[q2] = color;
        leds[q3] = color;
        leds[q4] = color;

        // Advance the q values
        if (i % 8 == 7) {
            q1 += 18;
            q2 += 18;
            q3 += 46;
            q4 += 46;
        } else {
            q1 += 2;
            q2 += 2;
            q3 -= 2;
            q4 -= 2;
        }
    }

    FastLED.show();
}

void Grid::UpdateLedsTTT(const String &payload)
{
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

  unsigned char c; // character received by payload

  // q values represent the 4 led quadrants of a tile
  // each q value is initialized for tile[0] (aka boardState[0])
  int q1 = 0;
  int q2 = 1;
  int q3 = 30;
  int q4 = 31;

  // Iterates through the boardState (which contains a char[64]) 
  // and updates the corresponding 2x2 section for each tile.
  for (int i = 0; i < 64; i++) {

    // determine the color 
    switch (i) {
      case 0:
      case 1:
      case 2:{
        const char* cStr = boardState[i];
        if (cStr) {
          c = cStr[0];  // Get the first character
          if (c == '0') { c = 'w'; }
        } else {
          c = 'w';  // Default value in case of error
        }
        break;
      }
      case 8: {
          const char* cStr = boardState[i-3];
          if (cStr) {
            c = cStr[0];  // Get the first character
            if (c == '0') { c = 'w'; }
          } else {
            c = 'w';  // Default value in case of error
          }
        break;
      }
      case 9: {
        const char* cStr = boardState[i-5];
          if (cStr) {
            c = cStr[0];  // Get the first character
            if (c == '0') { c = 'w'; }
          } else {
            c = 'w';  // Default value in case of error
          }
        break;
      }
      case 10: {
        const char* cStr = boardState[i-7];
        if (cStr) {
          c = cStr[0];  // Get the first character
          if (c == '0') { c = 'w'; }
        } else {
          c = 'w';  // Default value in case of error
        }
        break;
      }
      case 16:
      case 17:
      case 18:{
        const char* cStr = boardState[i - 10];
        if (cStr) {
          c = cStr[0];
          if (c == '0') { c = 'w'; }
        } else {
          c = 'w';
        }
        break;
      }
      default: {
        c = '0';
        break;
      }
    }
    CRGB color = CharToColor(c);

    // assign the color to the correct 4 leds on the strip
    leds[q1] = color;
    leds[q2] = color;
    leds[q3] = color;
    leds[q4] = color;

    // modify q values so the next 4 leds 
    if (i % 8 == 7) 
    {
      q1 += 18;
      q2 += 18;
      q3 += 46;
      q4 += 46;
    }
    else
    {
      q1 += 2;
      q2 += 2;
      q3 -= 2;
      q4 -= 2;
    }
  }

  // have board light up with updated led colors
  FastLED.show();

}

void Grid::StartTTT()
{
  Serial.println("StartTTT");
  int q1 = 0;
  int q2 = 1;
  int q3 = 30;
  int q4 = 31;

  for (int i = 0; i < 64; i++) {
    Serial.println(i);

    // determine the color 
    switch (i) {
      case 0:
      case 1:
      case 2:
      case 8:
      case 9:
      case 10:
      case 16:
      case 17:
      case 18: {
        leds[q1] = CRGB(255, 255, 255);
        leds[q2] = CRGB(255, 255, 255);
        leds[q3] = CRGB(255, 255, 255);
        leds[q4] = CRGB(255, 255, 255);
        break;
      }
      default:{
        leds[q1] = CRGB(0, 0, 0);
        leds[q2] = CRGB(0, 0, 0);
        leds[q3] = CRGB(0, 0, 0);
        leds[q4] = CRGB(0, 0, 0);
        break;
      }
    }

    // modify q values so the next 4 leds 
    if (i % 8 == 7) 
    {
      q1 += 18;
      q2 += 18;
      q3 += 46;
      q4 += 46;
    }
    else
    {
      q1 += 2;
      q2 += 2;
      q3 -= 2;
      q4 -= 2;
    }
  }

  FastLED.show();
}

void Grid::LiveLedsTTT(CRGB* ledArray) 
{
  
  int q1 = 0;
  int q2 = 1;
  int q3 = 30;
  int q4 = 31;

  for (int i = 0; i < 64; i++) {
    // determine the color
    CRGB color;
    switch (i) {
      case 0:
      case 1:
      case 2: {
        color = ledArray[i];
        break;
      }
      case 8: {
        color = ledArray[i-3];
        break;
      }
      case 9: {
        color = ledArray[i-5];
        break;
      }
      case 10: {
        color = ledArray[i-7];
        break;
      }
      case 16:
      case 17:
      case 18: {
        color = ledArray[i-10];
        break;
      }
      default: {
        color = CRGB::Black;
        break;
      }
    }

    leds[q1] = color;
    leds[q2] = color;
    leds[q3] = color;
    leds[q4] = color;

    // modify q values so the next 4 leds 
    if (i % 8 == 7) 
    {
      q1 += 18;
      q2 += 18;
      q3 += 46;
      q4 += 46;
    }
    else
    {
      q1 += 2;
      q2 += 2;
      q3 -= 2;
      q4 -= 2;
    }
  }
  Serial.println(" ");

  FastLED.show();
  
}

void Grid::ClearLeds()
{
  FastLED.clear();
}

CRGB Grid::CharToColor(unsigned char c)
{
  CRGB color;
  if ((char)c != '0') {
    //Serial.println("\n-----Handling color-----");
    //Serial.println((char)c);
  }
  switch (c) {
    case 'R': // RED
      color = CRGB(255, 0, 0);
    case 'r': // RED
      color = CRGB(255, 0, 0);
      break;
    case 'g': // GREEN
      color = CRGB(0, 255, 0);
      break;
    case 'b': // BLUE
      color = CRGB(0, 0, 255);
      break;
    case 'p': // PURPLE
      //color = CRGB(255, 0, 255);
      color = CRGB::Purple;
      break;
    case 'y': // YELLOW
      //color = CRGB(255, 255, 0);
      color = CRGB::Yellow;
      break;
    case 'c': // CYAN
      //color = CRGB(0, 255, 255);
      color = CRGB::Cyan;
      break;
    case 'o':
      // ORANGE
      //color = CRGB(238, 73, 35);
      color = CRGB::White;
      break;
    case 'w': // WHITE
    case '1':
      //color = CRGB(255, 255, 255);
      color = CRGB::Black;
      break;
    case '0': // BLACK
      color = CRGB(0, 0, 0);
      break;
    default:
      color = CRGB(0, 0, 0);
      break;
  }
  return color;
}

void Grid::UpdateSensors()
{

  int sensorRaw[8][8];
  int index = 0;

  for (int row = 0; row < 8; row++) {
    // Drive the current row LOW, others HIGH
    for (int i = 0; i < 8; i++) {
      digitalWrite(rowPins[i], (i == row) ? LOW : HIGH);
    }

    for (int col = 0; col < 8; col++) {
      int sensorReading = 0;
      if (col < 5) {
        sensorReading = analogRead(colPins[col]);
      } 
      // Mux pins
      else {
        if (col == 5) {
          digitalWrite(MUX_SEL_0_PIN, LOW);
          digitalWrite(MUX_SEL_1_PIN, LOW);
        } 
        else if (col == 6) {
          digitalWrite(MUX_SEL_0_PIN, HIGH);
          digitalWrite(MUX_SEL_1_PIN, LOW);
        } 
        else { // col == 7
          digitalWrite(MUX_SEL_0_PIN, LOW);
          digitalWrite(MUX_SEL_1_PIN, HIGH);
        }
        delayMicroseconds(50);
        sensorReading = analogRead(33);
      }

      sensorRaw[row][col] = sensorReading;
      currSensors[index] = (sensorReading < hallSense) ? 1 : 0;

      index++;
    }

    delay(10);
  }

  /*
  Serial.println("Sensor Matrix (RAW analog values):");
  for (int row = 0; row < 8; row++)
  {
    for (int col = 0; col < 8; col++)
    {
      Serial.print(sensorRaw[row][col]);
      Serial.print(" ");
    }
    Serial.println();  // Move to next row
  }
  Serial.println();    // Blank line for readability
  delay(500);
  */

}


bool Grid::PiecesChanged()
{
  for (int i = 0; i < 64; i++) 
  {
    if (prevSensors[i] != currSensors[i]) {return true;}
  }
  return false;
}

void Grid::SaveReadings()
{
  for (int i = 0; i < 64; i++) 
  {
    prevSensors[i] = currSensors[i];
  }
}

const unsigned char* Grid::GetCurrSensors()
{
  return currSensors;
}

unsigned char Grid::GetCurrSensorsAt(int n) const
{
  return currSensors[n];
}
