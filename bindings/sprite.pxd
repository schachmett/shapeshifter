"""
Bindings for the sprites.h module.
"""

from libcpp cimport bool
from libcpp.string cimport string
from libcpp.map cimport map as cmap


cdef extern from "sprite.cc":
    pass
cdef extern from "sprite.h" namespace "Sprites":
    ctypedef string SpriteID

    cdef struct Point:
        Point(double, double) except +
        double x
        double y

    cdef struct Pixel:
        Pixel(char, char, char) except +
        char red
        char green
        char blue

    cpdef enum EdgeBehavior:        #cpdef makes it a PEP 435 enum
        LOOP_DIRECT = 2
        LOOP_INDIRECT = 3   # (default)
        BOUNCE = 4
        STOP = 5         # does not work fully (can still creep into edge)
        DISAPPEAR = 6

    cdef cppclass Sprite:
        Sprite() except +
        Sprite(string) except +
        Sprite(string, double) except +

        void setID(SpriteID)
        const SpriteID getID() const
        void setFilename(string)
        const string getFilename() const
        void setWidth(int)
        const int getWidth() const
        void setHeight(int)
        const int getHeight() const

        const Pixel getPixel(int, int) const
        void doStep()
        void setVisible(bool)
        bool getVisible() const

        void setPosition(const Point)
        const Point getPosition() const
        void setDirection(const double)
        const double getDirection()
        void setSpeed(const double)
        const double getSpeed() const
        void setEdgeBehavior(const EdgeBehavior)
        const EdgeBehavior getEdgeBehavior() const

    ctypedef cmap[SpriteID, Sprite*] SpriteList


cdef class PySprite:
    cdef Sprite* c_spr
    cdef bool _ptr_owner
    cdef bool _is_initialized
    @staticmethod
    cdef PySprite from_ptr(Sprite*, bool owner=*)


cdef class PySpriteList:
    cdef SpriteList c_sprl
    cdef py_sprites
