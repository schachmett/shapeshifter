# distutils: language = c++
# distutils: sources = sprite.cc, /usr/include/GraphicsMagick/Magick++.h
# cython: language_level=3
# pylint: skip-file

from shapeshifter cimport Point, Sprite, SpriteList
from shapeshifter cimport InitializeMagick


# This is super important: If it is not called, the script will to
# a segfault. Now it is called when importing shapeshifter.so
InitializeMagick(NULL)


cdef class PySprite:
    cdef Sprite* c_spr;

    def __cinit__(self, const char* fname):
        self.c_spr = new Sprite(fname)

    def show_position(self):
        p = self.c_spr[0].getPosition()
        print(f"Position: x={p.x}, y={p.y}")

    def set_position(self, double x, double y):
        cdef Point p
        p.x = x
        p.y = y
        self.c_spr[0].setPosition(p)

    def __dealloc__(self):
        del self.c_spr
