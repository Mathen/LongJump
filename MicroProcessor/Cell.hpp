//Description

#include "Color.hpp"

struct Cell
{
    Cell(unsigned int posX, unsigned int posY) 
        : x(posX), y(posY), piecePlaced(false), color(Color::BLACK) {}


    const unsigned int x;
    const unsigned int y;

    bool piecePlaced;

    Color color;
    //bool colorOn;
};