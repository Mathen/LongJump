// Grid class which stores a 2d array of 'Cell' objects and
// functions to interface with them

#pragma once

#include <vector>
#include <FastLED.h>

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

  //Set the color of a grid based on an RGB/HVS value
  void SetColor(unsigned int x, unsigned int y, CRGB color);
  void SetColor(unsigned int x, unsigned int y, CHSV color);

  //Returns if a piece is placed on that location on the grid
  bool GetPiecePlaced(unsigned int x, unsigned int y);

private:
  //-----------Helper Functions----------
  void SetUpGrid();

  //-----------Parameters----------------
private:
  const unsigned int dimX;
  const unsigned int dimY;

  std::vector<CRGB> leds;  // LED array
  std::vector<std::vector<bool>> piecesPlaced;
};