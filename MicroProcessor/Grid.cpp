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
    int outPins[3] = {0, 1, 2};
    int inPins[3] = {3, 4, 5};

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
            GetCell(x, y)->piecePlaced = digitalRead(inPins[y]) == HIGH;
        }
    }
    
}

void Grid::ClearLeds()
{
    for (unsigned int x = 0; x < dimX; x++)
    {
        for (unsigned int y = 0; y < dimY; y++)
        {
            GetCell(x, y)->color = Color::BLACK;
        }
    }
}

Cell* Grid::operator()(unsigned int x, unsigned int y)
{
    GetCell(x, y);
}
Cell* Grid::GetCell(unsigned int x, unsigned int y)
{
    if (x < dimX && y < dimY)
        return &grid[x][y];
    else
        return nullptr;
}

void Grid::SetUpGrid()
{
    grid = std::vector<std::vector<Cell>>(dimX, std::vector<Cell>());

    for (unsigned int x = 0; x < dimX; x++)
    {
        for (unsigned int y = 0; y < dimY; y++)
        {
            grid[x].push_back(Cell(x, y));
        }
    }
}