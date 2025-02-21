// Grid class which stores a 2d array of 'Cell' objects and
// functions to interface with them

#pragma once

#include <vector>
#include <Adafruit_NeoPixel.h>
#include <string>

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
  void UpdateLeds(const String &payload, Adafruit_NeoPixel &strip);

  //Change all Cell LEDs to Black
  void ClearLeds(Adafruit_NeoPixel &strip);

  //Converts char to corresponding color
  uint32_t CharToColor(char c, Adafruit_NeoPixel &strip);

  //Set the color of a grid based on an RGB value
  //void SetColor(unsigned int x, unsigned int y, uint32_t color);
  //void SetColor(unsigned int x, unsigned int y, uint8_t r, uint8_t g, uint8_t b);

  //Returns if a piece is placed on that location on the grid
  //bool GetPiecePlaced(unsigned int x, unsigned int y);

  //Updates currSensors with hall effect sensor readings
  void UpdateSensors();

  //Compares currSensors with prevSensors
  bool PiecesChanged();

  //Overwrites prevSensors with currSensors
  void SaveReadings();

  //Returns currSensors
  const bool* GetCurrSensors();

  //Returns currSensors
bool GetCurrSensorsAt(int n) const;


private:
  //-----------Helper Functions----------
  //void SetUpGrid();

  //-----------Parameters----------------
private:
  unsigned int dimX;
  unsigned int dimY;

  bool prevSensors[64] = {0};
  bool currSensors[64] = {0};

  //std::vector<std::vector<bool>> piecesPlaced;
};