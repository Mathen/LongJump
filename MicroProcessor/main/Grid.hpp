// Grid class which stores a 2d array of 'Cell' objects and
// functions to interface with them

#pragma once

#include <vector>
#include <Adafruit_NeoPixel.h>

class Grid
{
public:
  //-----------Constructors-------------
  //Default Constructor
  Grid();

  //dimention: grid of size [dimention x dimention]
  Grid(unsigned int dimention);

  //dimentionX, dimentionY: grid of size [dimentionX x dimentionY]
  Grid(unsigned int dimentionX, unsigned int dimentionY);

  //-----------Grid Interface------------
  //Input piece placement data from circuit and change Cell LEDs
  void Update();

  //Change all Cell LEDs to Black
  void ClearLeds();

  //Set the color of a grid based on an RGB value
  void SetColor(unsigned int x, unsigned int y, uint32_t color);
  void SetColor(unsigned int x, unsigned int y, uint8_t r, uint8_t g, uint8_t b);

  //Returns if a piece is placed on that location on the grid
  bool GetPiecePlaced(unsigned int x, unsigned int y);

private:
  //-----------Helper Functions----------
  void SetUpGrid();

  //-----------Parameters----------------
private:
  unsigned int dimX;
  unsigned int dimY;

  Adafruit_NeoPixel leds;
  std::vector<std::vector<bool>> piecesPlaced;
};