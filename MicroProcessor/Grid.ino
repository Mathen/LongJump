#include "Grid.hpp"

// TODO: Define row and column pin values
const int rowPins[8] = {1, 2, 3, 4, 5, 6, 7, 8};
const int colPins[8] = {9, 10, 11, 12, 13, 14, 15, 16};
const int hallSense = 200;

/*
#define ROW0      18
#define ROW1      4
#define ROW2      19

#define COL0      34
#define COL1      35
#define COL2      32
*/

Grid::Grid() : dimX(0), dimY(0)
{
  //SetUpGrid();
}
Grid::Grid(unsigned int dimention) : dimX(dimention), dimY(dimention)
{
  //SetUpGrid();
}
Grid::Grid(unsigned int dimentionX, unsigned int dimentionY) : dimX(dimentionX), dimY(dimentionY)
{
  //SetUpGrid();
}

void Grid::UpdateLeds(const String &payload, Adafruit_NeoPixel &strip)
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

  char c; // character received by payload

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
    color = charToColor(c);

    // assign the color to the correct 4 leds on the strip
    strip.setPixelColor(q1, color);
    strip.setPixelColor(q2, color);
    strip.setPixelColor(q3, color);
    strip.setPixelColor(q4, color);

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
  strip.show();

}

void Grid::ClearLeds(Adafruit_NeoPixel &strip)
{
  strip.clear();
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

uint32_t Grid::CharToColor(char c, Adafruit_NeoPixel &strip)
{
  uint32_t color;
  switch (c) {
    case 'R': // RED
      color = strip.Color(255, 0, 0);
      break;
    case 'G': // GREEN
      color = strip.Color(0, 255, 0);
      break;
    case 'B': // BLUE
      color = strip.Color(0, 0, 255);
      break;
    case 'P': // PURPLE
      color = strip.Color(255, 0, 255);
      break;
    case 'Y': // YELLOW
      color = strip.Color(255, 255, 0);
      break;
    case 'C': // CYAN
      color = strip.Color(0, 255, 255);
      break;
    case 'W': // WHITE
      color = strip.Color(255, 255, 255);
      break;
    case '0': // BLACK
    case default:
      color = strip.Color(0, 0, 0);
      break;
  }
  return color;
}

void Grid::UpdateSensors()
{
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