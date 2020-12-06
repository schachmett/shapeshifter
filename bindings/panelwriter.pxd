"""
Bindings for the main loop that runs the panel animations.
"""

# from .panel cimport RGBMatrix
from .sprite cimport CanvasObjectList

from libcpp cimport bool
from libc.stdint cimport uint8_t, uint32_t

cdef extern from "canvas.h" namespace "rgb_matrix":
    cdef cppclass Canvas:
        int width()
        int height()
        # void SetPixel(int, int, unit8_t, uint8_t, uint8_t) nogil
        void Clear() nogil
        # void Fill(uint8_t, uint8_t, uint8_t) nogil
        void RGBPrintHWM()

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
        bool do_gpio_init

    RGBMatrix* CreateMatrixFromOptions(Options, RuntimeOptions)

cdef extern from "led-matrix.h" namespace "rgb_matrix::RGBMatrix":
    cdef struct Options:
        Options() except +
        const char* hardware_mapping
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
        const char* led_rgb_sequence
        const char* pixel_mapper_config
        const char* panel_type


cdef class PanelOptions:
    cdef Options __options
    cdef RuntimeOptions __rt_options
    cdef bytes __py_encoded_hardware_mapping
    cdef bytes __py_encoded_led_rgb_sequence
    cdef bytes __py_encoded_pixel_mapper_config
    cdef bytes __py_encoded_panel_type

cdef class PyRGBPanel:
    cdef PanelOptions options
    cdef RGBMatrix* __matrix
    cdef Canvas* __getCanvas(self) except *


cdef extern from "led-loop.cc":
    pass
cdef extern from "led-loop.h" namespace "led_loop":
    ctypedef long long int tmillis_t

    cdef struct LoopOptions:
        LoopOptions() except +
        tmillis_t frame_time_ms

    cdef cppclass AnimationLoop:
        AnimationLoop(RGBMatrix*, CanvasObjectList*, LoopOptions*) except +
        void startLoop()
        void endLoop()

cdef class PyAnimationLoop:
    cdef AnimationLoop* c_sal
    cdef PyRGBPanel rgb
    cdef CanvasObjectList* c_sprl
