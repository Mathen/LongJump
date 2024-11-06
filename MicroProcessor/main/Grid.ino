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
  //Pins
  int outPins[3] = {33, 32, 25};
  int inPins[3] = {34, 39, 36};

  //---Input pieces---
  //Cycle through outPins (turn one on at a time)
  //  Wait short time for next reads
  //  Read each inPins
  //  Set Cell(x, y) to value
  for (unsigned int x = 0; x < dimX; x++)
  {
    delay(10); //10ms
    for (unsigned int tempX = 0; tempX < dimX; tempX++)
    {
      if (tempX == x)
        digitalWrite(outPins[tempX], HIGH);
      else
        digitalWrite(outPins[tempX], LOW);
    }

    delay(10); //10ms
    for (unsigned int y = 0; y < dimY; y++)
    {
      piecesPlaced[x][y] = digitalRead(inPins[y]) == HIGH;
    }
  }
    
}

void Grid::ClearLeds()
{
  for (unsigned int x = 0; x < dimX; x++)
  {
    for (unsigned int y = 0; y < dimY; y++)
    {
      SetColor(x, y, CRGB::Black);
    }
  }
}

void SetColor(unsigned int x, unsigned int y, CRGB color)
{
  if (x < dimX && y < dimY)
    return false;

  if (y % 2 == 1)
    y = dimY - y - 1;

  leds[y * dimX + x] = color;

  // 0 1 2
  // 5 4 3
  // 6 7 8
}
void SetColor(unsigned int x, unsigned int y, CHVS color)
{
  CRGB rgb = color;
  SetColor(x, y, rgb);
}

bool GetPiecePlaced(unsigned int x, unsigned int y)
{
  if (x < dimX && y < dimY)
    return false;
  else
    return piecesPlaced[x][y];
}

void Grid::SetUpGrid()
{
  leds = std::vector<CRGB>(dimX * dimY, CRGB::Black);
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