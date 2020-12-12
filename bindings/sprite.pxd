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
        int getWidth() const
        int getHeight() const

        void setContent(string)
        const string getContent() const

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

        int getWidth()
        void setWidth(int)
        int getHeight()
        void setHeight(int)
        void setRotation(double)
        const double getRotation() const

        const Pixel getPixel(int, int) const

    cdef cppclass Text(CanvasObject):
        Text() except +
        Text(string) except +
        Text(string, string) except +

        void setText(string)
        const string getText() const
        void setKerning(int)
        const int getKerning() const

    ctypedef cmap[CanvasObjectID, CanvasObject*] CanvasObjectList
    ctypedef cmap[CanvasObjectID, CanvasObject*].iterator CanvasObjectListIterator

cdef class PyCanvasObject:
    cdef CanvasObject* c_cvo
    cdef CanvasObject* _cvo(self)
    cdef bool _ptr_owner
    cdef bool _is_initialized

cdef class PySprite(PyCanvasObject):
    cdef Sprite* c_spr
    @staticmethod
    cdef PySprite from_ptr(Sprite*, bool owner=*)

cdef class PyText(PyCanvasObject):
    cdef Text* c_txt
    @staticmethod
    cdef PyText from_ptr(Text*, bool owner=*)


cdef class PyCanvasObjectListBase:
    cdef CanvasObjectList c_cvos
    cdef py_sprites
    cdef cvo2py(self, CanvasObject*)
    cdef it2py(self, CanvasObjectListIterator)
