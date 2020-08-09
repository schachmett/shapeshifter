"""
Bindings for the sprites.h module.
"""

from libcpp cimport bool
from libcpp.string cimport string
from libcpp.map cimport map as cmap


cdef extern from "sprite.cc":
    pass
cdef extern from "sprite.h" namespace "Sprites":
    ctypedef string CanvasObjectID

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

    cdef cppclass CanvasObject:
        CanvasObject() except +

        void setID(CanvasObjectID)
        const CanvasObjectID getID() const
        const int getWidth() const
        const int getHeight() const

        void doStep()

        void setVisible(bool)
        bool getVisible() const
        void setEdgeBehavior(const EdgeBehavior)
        const EdgeBehavior getEdgeBehavior() const

        void setPosition(const Point)
        const Point getPosition() const
        void setDirection(const double)
        const double getDirection()
        void setSpeed(const double)
        const double getSpeed() const

    cdef cppclass Sprite(CanvasObject):
        Sprite() except +
        Sprite(string) except +
        Sprite(string, double) except +

        void setSource(string)
        const string getSource() const
        void setWidth(int)
        void setHeight(int)

        const Pixel getPixel(int, int) const

    ctypedef cmap[CanvasObjectID, Sprite*] SpriteList


cdef class PySprite:
    cdef Sprite* c_spr
    cdef bool _ptr_owner
    cdef bool _is_initialized
    @staticmethod
    cdef PySprite from_ptr(Sprite*, bool owner=*)


cdef class PySpriteList:
    cdef SpriteList c_sprl
    cdef py_sprites
