#include "LcdInterface.hpp"
#include "Grid.hpp"

Grid grid;

void setup()
{
  // put your setup code here, to run once:

  //Pins being used
  //Set Pins not being used to input (to not accidently make a short)
  pinMode( 0, OUTPUT);  // Out: grid piece placement row[0]
  pinMode( 1, OUTPUT);  // Out: grid piece placement row[1]
  pinMode( 2, OUTPUT);  // Out: grid piece placement row[2]
  pinMode( 3, INPUT);   //  In: grid piece placement col[0]
  pinMode( 4, INPUT);   //  In: grid piece placement col[1]
  pinMode( 5, INPUT);   //  In: grid piece placement col[2]
  pinMode( 6, INPUT);   // None
  pinMode( 7, INPUT);   // None
  pinMode( 8, INPUT);   // None
  pinMode( 9, INPUT);   // None
  pinMode(10, INPUT);   // None
  pinMode(11, INPUT);   // None
  pinMode(12, INPUT);   // None
  pinMode(13, INPUT);   // None
  pinMode(A0, INPUT);   // None
  pinMode(A0, INPUT);   // None
  pinMode(A1, INPUT);   // None
  pinMode(A2, INPUT);   // None
  pinMode(A3, INPUT);   // None
  pinMode(A4, INPUT);   // None
  pinMode(A5, INPUT);   // None

  //Grid
  grid = Grid(3);

  //LCD

}

void loop()
{
  // put your main code here, to run repeatedly:
  grid.Update();

  //Change Cell(1, 1) to output LED white
  if(grid(1, 1) != nullptr)
  {
    grid(1, 1)->color = Color::WHITE;
    // or 
    grid.GetCell(1, 1)->color = Color::WHITE;
  }
    
}