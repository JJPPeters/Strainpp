#ifndef COORD2D
#define COORD2D

// really simple container just to facilitate passing coordinates around
template <typename T>
class Coord2D
{
public:
    Coord2D(T a, T b) : x(a), y(b){}

    T x, y;

};

#endif // COORD2D

