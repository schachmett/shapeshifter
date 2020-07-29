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
        UNDEFINED_EDGE_BEHAVIOR = 1
        LOOP_DIRECT = 2
        LOOP_INDIRECT = 3
        BOUNCE = 4
        STOP = 5
        DISAPPEAR = 6

    cdef cppclass Sprite:
        Sprite() except +
        Sprite(SpriteID) except +
        Sprite(const char*) except +
        Sprite(SpriteID, const char*) except +
        void setID(SpriteID)
        const SpriteID getID() const
        const string getFilename() const
        const Pixel getPixel(int, int) const
        void doStep()
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
