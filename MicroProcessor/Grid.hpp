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
    bool Update();

    bool SetLed(unsigned int x, unsigned int y, const Color& c);
    bool ClearLeds();

    bool GetPiecePlaced(unsigned int x, unsigned int y, bool& isPlaced);

    //-----------Parameters----------------
public:
    unsigned int myDimX;
    unsigned int myDimY;

    std::vector<std::vector<Cell>> myGrid;
};