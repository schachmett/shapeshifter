# distutils: language = c++
# distutils: sources = sprite.cc, /usr/include/GraphicsMagick/Magick++.h
# cython: language_level=3

from libcpp cimport bool
from cython.operator cimport dereference as deref
from cython.operator cimport address

# from .sprite cimport Point, Sprite, SpriteList, EdgeBehavior
from .utility cimport pystr_to_chars, cstr_to_pystr
from .magick cimport InitializeMagick


# This is super important: If it is not called, executing any magick++ code
# will lead to a segfault. Now it is called when importing this module
# (aka shapeshifter.so)
InitializeMagick(NULL)


cdef class PySpriteList:
    # cdef SpriteList c_sprl
    # cdef py_sprites

    def __cinit__(self, **dict_of_pysprites):
        self.py_sprites = {}
        # cdef Py_ssize_t i
        # for i, _ in enumerate(sprites):
        #     self.c_sprl[str(i)] = sprites[i]

    def __init__(self, **dict_of_pysprites):
        for key, py_sprite in dict_of_pysprites.items():
            self.__setitem__(key, py_sprite)

    def __getitem__(self, str key):
        # cdef Sprite* c_sprite = self.c_sprl[str_to_chars(key)]
        # py_sprite = PySprite.from_ptr(c_sprite)
        py_sprite = self.py_sprites[key]
        return py_sprite

    def __setitem__(self, str key, PySprite sprite):
        if self.c_sprl.find(pystr_to_chars(key)) != self.c_sprl.end():
            print("overwriting")
        if sprite.c_spr.getID() != pystr_to_chars(key):
            sprite.c_spr.setID(pystr_to_chars(key))
            # print(f"Set ID {cstr_to_pystr(sprite.c_spr.getID())} to {key}")
        # cdef Sprite* c_spr = new Sprite(_str_to_chars(sprite.fname))
        self.c_sprl[pystr_to_chars(key)] = sprite.c_spr
        self.py_sprites[key] = sprite


cdef class PySprite:
    # cdef Sprite* c_spr
    # cdef bool _ptr_owner
    # cdef bool _is_initialized

    def __cinit__(self, str fname, str ID=""):
        if fname == "":
            self._is_initialized = False
            return
        self._is_initialized = True
        self._ptr_owner = True
        if ID == "":
            self.c_spr = new Sprite(pystr_to_chars(fname))
        else:
            self.c_spr = new Sprite(pystr_to_chars(ID), pystr_to_chars(fname))

    @staticmethod
    cdef PySprite from_ptr(Sprite* sprite, bool owner=False):
        """
        See also:
        https://cython.readthedocs.io/en/latest/src/userguide/
        extension_types.html#existing-pointers-instantiation
        """
        cdef PySprite py_sprite = PySprite.__new__(PySprite, "")
        py_sprite.c_spr = sprite
        py_sprite._ptr_owner = owner
        py_sprite._is_initialized = True
        return py_sprite

    def __dealloc__(self):
        if self._ptr_owner:
            del self.c_spr

    def do_step(self):
        deref(self.c_spr).doStep()

    def print_status(self):
        print(
            f"Sprite '{self.ID}'\n"
            f"\tat x={self.position[0]}, y={self.position[1]}\n"
            f"\twith speed {self.speed}, direction {self.direction}\n"
            f"\thas EdgeBehavior {self.edge_behavior}"
        )

    @property
    def ID(self):
        return deref(self.c_spr).getID().decode("UTF-8")
    @ID.setter
    def ID(self, value):
        deref(self.c_spr).setID(pystr_to_chars(value))

    @property
    def fname(self):
        return deref(self.c_spr).getFilename()

    def color(self, int x, int y):
        px = deref(self.c_spr).getPixel(x, y)
        return px.red, px.green, px.blue

    @property
    def position(self):
        p = deref(self.c_spr).getPosition()
        return p.x, p.y
    @position.setter
    def position(self, (double, double) value):
        cdef Point p
        p.x, p.y = value[0], value[1]
        deref(self.c_spr).setPosition(p)

    @property
    def direction(self):
        return deref(self.c_spr).getDirection()
    @direction.setter
    def direction(self, double value):
        deref(self.c_spr).setDirection(value)

    @property
    def speed(self):
        return deref(self.c_spr).getSpeed()
    @speed.setter
    def speed(self, double value):
        deref(self.c_spr).setSpeed(value)

    @property
    def edge_behavior(self):
        edge_behavior = deref(self.c_spr).getEdgeBehavior()
        return edge_behavior
    @edge_behavior.setter
    def edge_behavior(self, value):
        deref(self.c_spr).setEdgeBehavior(value)
