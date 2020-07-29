# distutils: language = c++
# distutils: sources = led-loop.cc
# cython: language_level=3

from cython.operator cimport dereference as deref

from .panel cimport PyRGBPanel, PanelOptions
from .sprite cimport PySpriteList

# from .loop cimport SpriteAnimationLoop, LoopOptions


cdef class PySpriteAnimationLoop:
    cdef SpriteAnimationLoop* c_sal
    cdef PyRGBPanel rgb

    def __cinit__(self, PySpriteList sprites, **options):
        print("### start")
        cdef LoopOptions cl_options = LoopOptions()
        # cdef PyRGBMatrixOptions matrix_opt = PyRGBMatrixOptions()
        rgb = PyRGBPanel(brightness=50)
        print(rgb.brightness)
        print("hi3")

        self.c_sal = new SpriteAnimationLoop(rgb.__matrix, &sprites.c_sprl, &cl_options)
        print("hi4")
