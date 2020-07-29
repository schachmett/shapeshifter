"""
This is mostly a copy of the contents of the two files:
- ../matrix/bindings/python/rgbmatrix/cppinc.pxd
- ../matrix/bindings/python/rgbmatrix/core.pxd
This way (without the extra cppinc module), the classes are easily accessible
from the pyx-files in this directory. The implementations for the python
classes at the bottom are in "matrix_core.pyx"
"""

from libcpp cimport bool
from libc.stdint cimport uint8_t, uint32_t

########################
### External classes ###
########################

cdef extern from "canvas.h" namespace "rgb_matrix":
    cdef cppclass Canvas:
        int width()
        int height()
        void SetPixel(int, int, uint8_t, uint8_t, uint8_t) nogil
        void Clear() nogil
        void Fill(uint8_t, uint8_t, uint8_t) nogil

cdef extern from "led-matrix.h" namespace "rgb_matrix":
    cdef cppclass RGBMatrix(Canvas):
        bool SetPWMBits(uint8_t)
        uint8_t pwmbits()
        void set_luminance_correct(bool)
        bool luminance_correct()
        void SetBrightness(uint8_t)
        uint8_t brightness()
        FrameCanvas *CreateFrameCanvas()
        FrameCanvas *SwapOnVSync(FrameCanvas*)

    cdef cppclass FrameCanvas(Canvas):
        bool SetPWMBits(uint8_t)
        uint8_t pwmbits()
        void SetBrightness(uint8_t)
        uint8_t brightness()

    struct RuntimeOptions:
      RuntimeOptions() except +
      int gpio_slowdown
      int daemon
      int drop_privileges


    RGBMatrix *CreateMatrixFromOptions(Options &options, RuntimeOptions runtime_options)



cdef extern from "led-matrix.h" namespace "rgb_matrix::RGBMatrix":
    cdef struct Options:
        Options() except +

        const char *hardware_mapping

        int rows
        int cols
        int chain_length
        int parallel
        int pwm_bits
        int pwm_lsb_nanoseconds
        int brightness
        int scan_mode
        int row_address_type
        int multiplexing
        int pwm_dither_bits
        int limit_refresh_rate_hz

        bool disable_hardware_pulsing
        bool show_refresh_rate
        bool inverse_colors

        const char *led_rgb_sequence
        const char *pixel_mapper_config
        const char *panel_type


cdef class PyCanvas:
    cdef Canvas *__getCanvas(self) except +

cdef class PyFrameCanvas(PyCanvas):
    cdef FrameCanvas *__canvas

cdef class PyRGBMatrix(PyCanvas):
    cdef RGBMatrix *__matrix

cdef class PyRGBMatrixOptions:
    cdef Options __options
    cdef RuntimeOptions __runtime_options
    # Must keep a reference to the encoded bytes for the strings,
    # otherwise, when the Options struct is used, it will be garbage collected
    cdef bytes __py_encoded_hardware_mapping
    cdef bytes __py_encoded_led_rgb_sequence
    cdef bytes __py_encoded_pixel_mapper_config
    cdef bytes __py_encoded_panel_type

# Local Variables:
# mode: python
# End:
