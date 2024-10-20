struct Color
{
    Color() : r(0), g(0), b(0) {}
    constexpr Color(unsigned char red, unsigned char green, unsigned char blue)
        : r(red), g(green), b(blue) {}

    unsigned char r;
    unsigned char g;
    unsigned char b;

    //Fixed colors
    static const Color BLACK;
    static const Color WHITE;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
};

constexpr Color Color::BLACK = Color(  0,   0,   0);
constexpr Color Color::WHITE = Color(255, 255, 255);
constexpr Color Color::RED   = Color(255,   0,   0);
constexpr Color Color::GREEN = Color(  0, 255,   0);
constexpr Color Color::BLUE  = Color(  0,   0, 255);