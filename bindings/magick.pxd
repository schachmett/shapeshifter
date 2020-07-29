"""
Binding for the Magick++ initialization stuff.
"""

cdef extern from "Magick++.h" namespace "Magick":
    cdef void InitializeMagick(char[])
