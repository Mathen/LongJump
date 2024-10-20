#include "Grid.hpp"

Grid::Grid()
{

}
Grid::Grid(unsigned int dimention) : myDimX(dimention), myDimY(dimention)
{

}
Grid::Grid(unsigned int dimentionX, unsigned int dimentionY) : myDimX(dimentionX), myDimY(dimentionY)
{

}

bool Grid::Update()
{
    return true;
}

bool Grid::SetLed(unsigned int x, unsigned int y, const Color& c)
{
    return true;
}

bool Grid::ClearLeds()
{
    return true;
}

bool Grid::GetPiecePlaced(unsigned int x, unsigned int y, bool& isPlaced)
{
    return true;
}