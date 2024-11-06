#include "Grid.hpp"

Grid::Grid() : dimX(0), dimY(0)
{
  SetUpGrid();
}
Grid::Grid(unsigned int dimention) : dimX(dimention), dimY(dimention)
{
  SetUpGrid();
}
Grid::Grid(unsigned int dimentionX, unsigned int dimentionY) : dimX(dimentionX), dimY(dimentionY)
{
  SetUpGrid();
}

void Grid::Update()
{
  //Leds
  leds.show();


  //Pins
  int outPins[3] = {33, 32, 25};
  int inPins[3] = {34, 39, 36};

  //---Input pieces---
  //Cycle through outPins (turn one on at a time)
  //  Wait short time for next reads
  //  Read each inPins
  //  Set Cell(x, y) to value
  for (unsigned int y = 0; y < dimY; y++)
  {
    delay(10); //10ms
    for (unsigned int tempY = 0; tempY < dimY; tempY++)
    {
      if (tempY == y)
        digitalWrite(outPins[tempY], HIGH);
      else
        digitalWrite(outPins[tempY], LOW);
    }

    delay(10); //10ms
    for (unsigned int x = 0; x < dimX; x++)
    {
      piecesPlaced[x][y] = digitalRead(inPins[y]) == HIGH;
    }
  }
}

void Grid::ClearLeds()
{
  leds.clear();
}

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

bool Grid::GetPiecePlaced(unsigned int x, unsigned int y)
{
  if (x < dimX && y < dimY)
    return false;
  else
    return piecesPlaced[x][y];
}

void Grid::SetUpGrid()
{
  leds.setPin(13);
  leds.updateLength(dimX * dimY);
  leds.setBrightness(0xFF);

  leds.begin();
  ClearLeds();

  piecesPlaced = std::vector<std::vector<bool>>(dimX, std::vector<bool>());
  for (unsigned int x = 0; x < dimX; x++)
  {
    for (unsigned int y = 0; y < dimY; y++)
    {
        piecesPlaced[x].push_back(false);
    }
  }
}