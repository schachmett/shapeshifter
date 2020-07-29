"""
Helper functions.
"""

from libcpp.string cimport string as std_string
from .magick cimport InitializeMagick

cdef inline const char* pystr_to_chars(str py_string):
    byte_string = py_string.encode("UTF-8")
    cdef const char* c_fname = byte_string
    return c_fname

cdef inline cstr_to_pystr(std_string string):
    cdef const char* c_chars = string.c_str()
    return c_chars.decode("UTF-8")

# cdef chars_to_pystr(const char* c_chars):
#     return c_chars.decode("UTF-8")
