#include "LcdInterface.hpp"
#include "Grid.hpp"

Grid myGrid;

void setup()
{
  // put your setup code here, to run once:
  myGrid = Grid(3);

}

void loop()
{
  // put your main code here, to run repeatedly:
  myGrid.Update();
}