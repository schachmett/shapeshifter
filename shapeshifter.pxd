# pylint: skip-file

cdef extern from "sprite.cc":
    pass

cdef extern from "sprite.h" namespace "Sprites":
    ctypedef SpriteID
    ctypedef SpriteList

    cdef struct Point:
        Point(double, double) except +
        double x
        double y

    cdef cppclass Sprite:
        Sprite(const char*) except +
        void doStep()
        void setPosition(const Point)
        Point getPosition()

cdef extern from "Magick++.h" namespace "Magick":
    cdef void InitializeMagick(char[])
