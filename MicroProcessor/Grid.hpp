// Grid class which stores a 2d array of 'Cell' objects and
// functions to interface with them

#include "Cell.hpp"
#include "Color.hpp"
#include <vector>

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
    //All return true if successful

    //Input piece placement data from circuit and change Cell LEDs
    void Update();

    //Change all Cell LEDs to Black
    void ClearLeds();

    //Access Cell* from grid. Ex: grid(x, y)
    Cell* operator()(unsigned int x, unsigned int y);
    Cell* GetCell(unsigned int x, unsigned int y);

private:
    //-----------Helper Functions----------
    void SetUpGrid();

    //-----------Parameters----------------
private:
    unsigned int dimX;
    unsigned int dimY;

    std::vector<std::vector<Cell>> grid;
};