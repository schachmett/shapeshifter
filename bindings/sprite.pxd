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

        void setSource(string)
        const string getSource() const

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

        # void setSource(string)
        # const string getSource() const
        void setWidth(int)
        void setHeight(int)

        const Pixel getPixel(int, int) const

    cdef cppclass Text(CanvasObject):
        Text() except +
        Text(string) except +
        Text(string, string) except +

        # void setSource(string, int)
        # const string getSource() const
        void setText(string)
        const string getText() const
        void setKerning(int)
        const int getKerning() const

    ctypedef cmap[CanvasObjectID, CanvasObject*] CanvasObjectList

cdef class PyCanvasObject:
    cdef CanvasObject* c_cvo
    cdef bool _ptr_owner
    cdef bool _is_initialized
    cdef CanvasObject* _wrapped_CVO(self)

cdef class PySprite(PyCanvasObject):
    cdef Sprite* c_spr
    @staticmethod
    cdef PySprite from_ptr(Sprite*, bool owner=*)
    cdef CanvasObject* _wrapped_CVO(self)
    cdef Sprite* _wrapped(self)

cdef class PyText(PyCanvasObject):
    cdef Text* c_txt
    @staticmethod
    cdef PyText from_ptr(Text*, bool owner=*)
    cdef CanvasObject* _wrapped_CVO(self)
    cdef Text _wrapped(self)


cdef class PyCanvasObjectList:
    cdef CanvasObjectList c_sprl
    cdef py_sprites
