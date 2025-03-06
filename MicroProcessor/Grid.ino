#include "Grid.hpp"

// TODO: Define row and column pin values
const int rowPins[8] = {18, 2, 3, 4, 5, 6, 7, 8};
const int colPins[8] = {34, 10, 11, 12, 13, 14, 15, 16};
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

void Grid::UpdateLeds(const String &payload)
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
    c = boardState[i];
    CRGB color = CharToColor(c);

    // prints 1d array boardState sent by MQTT server
    Serial.print(c);
    if (i < 63) { Serial.print(", "); }
    else { Serial.println(" "); }

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

void Grid::ClearLeds()
{
  FastLED.clear();
}

/*
void Grid::SetColor(unsigned int x, unsigned int y, uint32_t color)
{
  if (x < dimX && y < dimY)
    return;

  if (y % 2 == 1)
    y = dimY - y - 1;

  leds.setPixelColor(y * dimX + x, color);

  // 0 1 2
  // 5 4 3
  // 6 7 8
}
void Grid::SetColor(unsigned int x, unsigned int y, uint8_t r, uint8_t g, uint8_t b)
{
  SetColor(x, y, Adafruit_NeoPixel::Color(r, g, b));
}
*/

/*
bool Grid::GetPiecePlaced(unsigned int x, unsigned int y)
{
  if (x < dimX && y < dimY)
    return false;
  else
    return piecesPlaced[x][y];
}
*/

CRGB Grid::CharToColor(unsigned char c)
{
  CRGB color;
  switch (c) {
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
      color = CRGB(255, 0, 255);
      break;
    case 'y': // YELLOW
      color = CRGB(255, 255, 0);
      break;
    case 'c': // CYAN
      color = CRGB(0, 255, 255);
      break;
    case 'w': // WHITE
      color = CRGB(255, 255, 255);
      break;
    case '0': // BLACK
    default:
      color = CRGB(0, 0, 0);
      break;
  }
  return color;
}

void Grid::UpdateSensors()
{
  digitalWrite(rowPins[0], LOW);
  int sensorReading = analogRead(colPins[0]);
  if (sensorReading < hallSense) { currSensors[0] = false; }
  else { currSensors[0] = true; }

  /*
  // will iterate 0-63 for each sensor
  int num = 0;

  // iterates through rows
  for (int row = 0; row < 8; row++) 
  {
    // sets row pins LOW and HIGH appropriately
    for (int i = 0; i < 8; i++) 
    {
      if (i == row) { digitalWrite(rowPins[i], LOW); }
      else { digitalWrite(rowPins[i], HIGH); }
    }

    // iterates through columns
    for (int col = 0; col < 8; col++)
    {
      // Assigns a boolean value to currSensors[num] based on readings 
      // taken for the current iteration of row and col.
      int sensorReading = analogRead(colPins[col]);
      if (sensorReading < hallSense) { currSensors[num] = false; }
      else { currSensors[num] = true; }

      // Increment or decrement num based on the current row
      if (row % 2 == 0) { num++; }
      else { num--; }
    }

    // Increment num the correct amount based on the current row
    if (row % 2 == 0) { num += 7; }
    else { num += 9; }

    delay(5);
  }
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

const bool* Grid::GetCurrSensors()
{
  return currSensors;
}

bool Grid::GetCurrSensorsAt(int n) const
{
  return currSensors[n];
}

/*
void Grid::SetUpGrid()
{
  //leds = Adafruit_NeoPixel(dimX * dimY, 22, NEO_GRB + NEO_KHZ800);
  //leds.setPin(22);
  //leds.updateLength(dimX * dimY);
  //leds.setBrightness(0xFF);

  //leds.begin();
  //ClearLeds();

  piecesPlaced = std::vector<std::vector<bool>>(dimX, std::vector<bool>());
  for (unsigned int x = 0; x < dimX; x++)
  {
    for (unsigned int y = 0; y < dimY; y++)
    {
        piecesPlaced[x].push_back(false);
    }
  }
}
*/
