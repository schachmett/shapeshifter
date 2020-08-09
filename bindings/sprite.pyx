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

    def __delitem__(self, str key):
        raise NotImplementedError

    def __iter__(self):
        return iter(self.py_sprites)

    def values(self):
        return self.py_sprites.values()

    def keys(self):
        return self.py_sprites.keys()

    def __len__(self):
        return len(self.py_sprites)


cdef class PySprite:
    # cdef Sprite* c_spr
    # cdef bool _ptr_owner
    # cdef bool _is_initialized

    def __cinit__(self, str fname):
        if fname == "":
            self._is_initialized = False
            return
        self._is_initialized = True
        self._ptr_owner = True
        self.c_spr = new Sprite(pystr_to_chars(fname))

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

    property ID:
        def __get__(self): return deref(self.c_spr).getID().decode("UTF-8")
        def __set__(self, str value): deref(self.c_spr).setID(pystr_to_chars(value))

    property fname:
        def __get__(self): return deref(self.c_spr).getSource()
        def __set__(self, str value): deref(self.c_spr).setSource(value)

    property visible:
        def __get__(self): return deref(self.c_spr).getVisible()
        def __set__(self, bool value): deref(self.c_spr).setVisible(value)

    property width:
        def __get__(self): return deref(self.c_spr).getWidth()
        def __set__(self, int value): deref(self.c_spr).setWidth(value)

    property height:
        def __get__(self): return deref(self.c_spr).getHeight()
        def __set__(self, int value): deref(self.c_spr).setHeight(value)

    property position:
        def __get__(self):
            p = deref(self.c_spr).getPosition()
            return p.x, p.y
        def __set__(self, (double, double) value):
            cdef Point p
            p.x, p.y = value[0], value[1]
            deref(self.c_spr).setPosition(p)

    property direction:
        def __get__(self): return deref(self.c_spr).getDirection()
        def __set__(self, double value): deref(self.c_spr).setDirection(value)

    property speed:
        def __get__(self): return deref(self.c_spr).getSpeed()
        def __set__(self, double value): deref(self.c_spr).setSpeed(value)

    property edge_behavior:
        def __get__(self): return deref(self.c_spr).getEdgeBehavior()
        def __set__(self, value): deref(self.c_spr).setEdgeBehavior(value)

    def color(self, int x, int y):
        px = deref(self.c_spr).getPixel(x, y)
        return px.red, px.green, px.blue

    def print_status(self):
        print(
            f"Sprite '{self.ID}' (visible: {self.visible})\n"
            f"\tat x={self.position[0]}, y={self.position[1]}\n"
            f"\twith speed {self.speed}, direction {self.direction}\n"
            f"\thas EdgeBehavior {self.edge_behavior}"
        )
