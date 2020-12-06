# distutils: language = c++
# distutils: sources = led-loop.cc
# cython: language_level=3
"""
Wrappers for RGBMatrix, Options (contains RGBMatrix::Options and RuntimeOptions)
and the AnimationLoop that writes to the matrix.
These can not be in separate files, because every FrameCanvas of the RGBMatrix
holds a Framebuffer, and the static class variable
Framebuffer::hardware_mapping_ is only visible inside the same module.
(If the loop is in another module, it sees a hardware_mapping_ of NULL).
"""

from cython.operator cimport dereference as deref
from libcpp cimport bool
from libc.stdint cimport uint8_t, uint32_t, uintptr_t

from .sprite cimport PyCanvasObjectList
# These are automatically imported from $0.pxd
# from .loop cimport (
#     AnimationLoop, LoopOptions,
#     Options, RuntimeOptions, RGBMatrix, FrameCanvas
# )


from .utility cimport pystr_to_chars, cstr_to_pystr

cdef class PyRGBPanel:
    def __cinit__(self, PanelOptions options=None, **kw_options):
        if options == None:
            options = PanelOptions(**kw_options)
        self.options = options
        self.__matrix = CreateMatrixFromOptions(
            options.__options,
            options.__rt_options
        )

    def __dealloc__(self):
        if <void*>self.__matrix != NULL:
            self.__matrix.Clear()
            del self.__matrix

    cdef Canvas* __getCanvas(self) except *:
        if <void*>self.__matrix != NULL:
            return self.__matrix
        raise Exception("Canvas was destroyed or not initialized")

    def test_cfc(self):
        self.__matrix.CreateFrameCanvas()

    property luminanceCorrect:
        def __get__(self): return self.__matrix.luminance_correct()
        def __set__(self, luminanceCorrect): self.__matrix.set_luminance_correct(luminanceCorrect)

    property pwmBits:
        def __get__(self): return self.__matrix.pwmbits()
        def __set__(self, pwmBits): self.__matrix.SetPWMBits(pwmBits)

    property brightness:
        def __get__(self): return self.__matrix.brightness()
        def __set__(self, brightness): self.__matrix.SetBrightness(brightness)

    property height:
        def __get__(self): return self.__matrix.height()

    property width:
        def __get__(self): return self.__matrix.width()


cdef class PanelOptions:
    def __cinit__(self, **options):

        DEFAULT_PANEL_OPTIONS = {
            "hardware_mapping": "regular",
            "rows": 64,
            "cols": 64,
            "chain_length": 3,
            "parallel": 1,
            "pwm_bits": 11,
            "pwm_lsb_nanoseconds": 130,
            "brightness": 100,
            "scan_mode": 1,         # try 0?
            "row_address_type": 0,
            "multiplexing": 0,
            "pwm_dither_bits": 0,   # better 1 (?)
            "limit_refresh_rate_hz": 0,
            "disable_hardware_pulsing": False,
            "show_refresh_rate": False,
            "inverse_colors": False,
            "led_rgb_sequence": "RGB",
            # "pixel_mapper_config": NULL,
            # "panel_type": NULL,
            # Runtime Opts
            "gpio_slowdown": 1,
            "daemon": 0,           # -1 = disabled
            "drop_privileges": 1,
            "do_gpio_init": True
        }
        self.__options = Options()
        self.__rt_options = RuntimeOptions()
        options = dict(DEFAULT_PANEL_OPTIONS, **options)
        for key, value in options.items():
            if not hasattr(self, key):
                continue
            setattr(self, key, value)

    # RGBMatrix options
    property hardware_mapping:
        def __get__(self): return self.__options.hardware_mapping
        def __set__(self, value):
            self.__py_encoded_hardware_mapping = value.encode("UTF-8")
            self.__options.hardware_mapping = self.__py_encoded_hardware_mapping

    property rows:
        def __get__(self): return self.__options.rows
        def __set__(self, uint8_t value): self.__options.rows = value

    property cols:
        def __get__(self): return self.__options.cols
        def __set__(self, uint8_t value): self.__options.cols = value

    property chain_length:
        def __get__(self): return self.__options.chain_length
        def __set__(self, uint8_t value): self.__options.chain_length = value

    property parallel:
        def __get__(self): return self.__options.parallel
        def __set__(self, uint8_t value): self.__options.parallel = value

    property pwm_bits:
        def __get__(self): return self.__options.pwm_bits
        def __set__(self, uint8_t value): self.__options.pwm_bits = value

    property pwm_lsb_nanoseconds:
        def __get__(self): return self.__options.pwm_lsb_nanoseconds
        def __set__(self, uint32_t value): self.__options.pwm_lsb_nanoseconds = value

    property brightness:
        def __get__(self): return self.__options.brightness
        def __set__(self, uint8_t value): self.__options.brightness = value

    property scan_mode:
        def __get__(self): return self.__options.scan_mode
        def __set__(self, uint8_t value): self.__options.scan_mode = value

    property multiplexing:
        def __get__(self): return self.__options.multiplexing
        def __set__(self, uint8_t value): self.__options.multiplexing = value

    property row_address_type:
        def __get__(self): return self.__options.row_address_type
        def __set__(self, uint8_t value): self.__options.row_address_type = value

    property disable_hardware_pulsing:
        def __get__(self): return self.__options.disable_hardware_pulsing
        def __set__(self, value): self.__options.disable_hardware_pulsing = value

    property show_refresh_rate:
        def __get__(self): return self.__options.show_refresh_rate
        def __set__(self, value): self.__options.show_refresh_rate = value

    property inverse_colors:
        def __get__(self): return self.__options.inverse_colors
        def __set__(self, value): self.__options.inverse_colors = value

    property led_rgb_sequence:
        def __get__(self): return self.__options.led_rgb_sequence
        def __set__(self, value):
            self.__py_encoded_led_rgb_sequence = value.encode("UTF-8")
            self.__options.led_rgb_sequence = self.__py_encoded_led_rgb_sequence

    property pixel_mapper_config:
        def __get__(self): return self.__options.pixel_mapper_config
        def __set__(self, value):
            self.__py_encoded_pixel_mapper_config = value.encode("UTF-8")
            self.__options.pixel_mapper_config = self.__py_encoded_pixel_mapper_config

    property panel_type:
        def __get__(self): return self.__options.panel_type
        def __set__(self, value):
            self.__py_encoded_panel_type = value.encode("UTF-8")
            self.__options.panel_type = self.__py_encoded_panel_type

    property pwm_dither_bits:
        def __get__(self): return self.__options.pwm_dither_bits
        def __set__(self, uint8_t value): self.__options.pwm_dither_bits = value

    property limit_refresh_rate_hz:
        def __get__(self): return self.__options.limit_refresh_rate_hz
        def __set__(self, value): self.__options.limit_refresh_rate_hz = value

    # Runtime options
    property gpio_slowdown:
        def __get__(self): return self.__rt_options.gpio_slowdown
        def __set__(self, uint8_t value): self.__rt_options.gpio_slowdown = value

    property daemon:
        def __get__(self): return self.__rt_options.daemon
        def __set__(self, int value): self.__rt_options.daemon = value

    property drop_privileges:
        def __get__(self): return self.__rt_options.drop_privileges
        def __set__(self, uint8_t value): self.__rt_options.drop_privileges = value


cdef class PyAnimationLoop:
    def __cinit__(self, PyCanvasObjectList sprites, **options):
        cdef LoopOptions cl_options = LoopOptions()
        if "frame_time_ms" in options:
            cl_options.frame_time_ms = options.pop("frame_time_ms")
        self.rgb = PyRGBPanel(**options)
        self.c_sprl = &sprites.c_sprl
        self.c_sal = new AnimationLoop(
            self.rgb.__matrix,
            self.c_sprl,
            &cl_options
        )

    def __dealloc__(self):
        del self.c_sal

    def start(self):
        print("Starting Animation Loop")
        deref(self.c_sal).startLoop()

    def end(self):
        print("Stopping Animation Loop")
        deref(self.c_sal).endLoop()
