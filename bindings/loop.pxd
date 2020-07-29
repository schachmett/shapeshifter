"""
Bindings for the main loop that runs the panel animations.
"""

from .panel cimport RGBMatrix
from .sprite cimport SpriteList


cdef extern from "led-loop.cc":
    pass
cdef extern from "led-loop.h" namespace "led_loop":
    ctypedef long long int tmillis_t

    cdef struct LoopOptions:
        LoopOptions() except +
        tmillis_t frame_time_ms

    cdef cppclass SpriteAnimationLoop:
        SpriteAnimationLoop(RGBMatrix*, SpriteList*, LoopOptions*) except +
        void startLoop() nogil
        void endLoop() nogil
