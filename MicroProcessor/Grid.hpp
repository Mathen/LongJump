#pragma once

#include <vector>
#include <FastLED.h>
#include <string>

class Grid
{
public:
  //-----------Constructors-------------
  //Default Constructor
  Grid();

  //dimention: grid of size [dimention x dimention]
  Grid(CRGB* ledArray, unsigned int dimention);

  //dimentionX, dimentionY: grid of size [dimentionX x dimentionY]
  Grid(CRGB* ledArray, unsigned int dimentionX, unsigned int dimentionY);

  //-----------Grid Interface------------
  //Input piece placement data from circuit and change Cell LEDs
  void UpdateLeds(const String &payload);

  //Input piece placement data from circuit and change Cell LEDs expecting 3x3 data
  void UpdateLedsTTT(const String &payload);

  void StartTTT();

  void LiveLedsTTT(CRGB* ledArray);

  //Change all Cell LEDs to Black
  void ClearLeds();

  //Converts char to corresponding color
  CRGB CharToColor(unsigned char c);

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
  const unsigned char* GetCurrSensors();

  //Returns currSensors
  unsigned char GetCurrSensorsAt(int n) const;


private:
  //-----------Helper Functions----------
  //void SetUpGrid();

  //-----------Parameters----------------
private:
  unsigned int dimX;
  unsigned int dimY;
  CRGB* leds;

  unsigned char prevSensors[64] = {0};
  unsigned char currSensors[64] = {0};

  //std::vector<std::vector<bool>> piecesPlaced;
};
