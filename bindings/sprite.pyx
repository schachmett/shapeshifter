# distutils: language = c++
# distutils: sources = sprite.cc, /usr/include/GraphicsMagick/Magick++.h
# cython: language_level=3

from libcpp cimport bool
from libcpp.typeinfo cimport type_info
from cython.operator cimport dereference as deref
from cython.operator cimport address
from cython.operator cimport typeid

# from .sprite cimport Point, Sprite, CanvasObjectList, EdgeBehavior
from .utility cimport pystr_to_chars, cstr_to_pystr
from .magick cimport InitializeMagick


# This is super important: If it is not called, executing any magick++ code
# will lead to a segfault. Now it is called when importing this module
# (aka shapeshifter.so)
InitializeMagick(NULL)

# cdef const type_info* info_sprite = &typeid(Sprite)
# cdef const type_info* info_text = &typeid(Text)


cdef class PyCanvasObjectList():
    # cdef CanvasObjectList c_sprl
    # cdef py_sprites

    def __cinit__(self, **dict_of_pysprites):
        self.py_sprites = {}

    # def __init__(self, **dict_of_pysprites):
    #     for key, py_sprite in dict_of_pysprites.items():
    #         self.__setitem__(key, py_sprite)

    def __getitem__(self, str key):
        it = self.c_sprl.find(pystr_to_chars(key))
        if it == self.c_sprl.end():
            raise KeyError
        cdef CanvasObject* c_cvo = deref(it).second
        if typeid(deref(c_cvo)) == typeid(Text):
            return PyText.from_ptr(<Text*>c_cvo)
        if typeid(deref(c_cvo)) == typeid(Sprite):
            return PySprite.from_ptr(<Sprite*>c_cvo)
        else:
            raise TypeError

        # return self.py_sprites.__getitem__(key)

    def __setitem__(self, str key, PyCanvasObject cv_obj):
        if self.c_sprl.find(pystr_to_chars(key)) != self.c_sprl.end():
            print("overwriting")
        cv_obj.ID = key
        cdef CanvasObject* c_cvo = cv_obj._cvo()
        self.c_sprl[pystr_to_chars(key)] = c_cvo

        self.py_sprites.__setitem__(key, cv_obj)

    def __delitem__(self, str key):
        it = self.c_sprl.find(pystr_to_chars(key))
        if it == self.c_sprl.end():
            raise KeyError
        else:
            self.c_sprl.erase(it)

        self.py_sprites.__delitem__(key)

    def __iter__(self):
        return self.py_sprites.__iter__()

    def __len__(self):
        return self.py_sprites.__len__()

    def __contains__(self, key):
        return key in self.py_sprites

    def keys(self):
        return self.py_sprites.keys()

    def values(self):
        return self.py_sprites.values()

    def items(self):
        return self.py_sprites.items()

    def get(self, key, *args, **kwargs):
        return self.py_sprites.get(key,  *args, **kwargs)

    # def values(self):
    #     return self.py_sprites.values()
    #
    # def keys(self):
    #     return self.py_sprites.keys()


cdef class PyCanvasObject:
    # cdef CanvasObject* c_cvo
    # cdef bool _ptr_owner
    # cdef bool _is_initialized
    # cdef CanvasObject* _cvo(self)

    def __cinit__(self, *args, **kwargs):
        self._is_initialized = False
        self._ptr_owner = True

    def __init__(self, *args, **kwargs):
        raise NotImplementedError("Should be abstract")

    def __dealloc__(self):
        if self._ptr_owner:
            del self.c_cvo

    cdef CanvasObject* _cvo(self):
        return self.c_cvo

    def do_step(self):
        self._cvo().doStep()

    property ID:
        def __get__(self): return self._cvo().getID().decode("UTF-8")
        def __set__(self, str value): self._cvo().setID(pystr_to_chars(value))

    property fname:
        def __get__(self): return self._cvo().getSource()
        def __set__(self, str value): self._cvo().setSource(value)

    property visible:
        def __get__(self): return self._cvo().getVisible()
        def __set__(self, bool value): self._cvo().setVisible(value)

    property width:
        def __get__(self): return self._cvo().getWidth()

    property height:
        def __get__(self): return self._cvo().getHeight()

    property position:
        def __get__(self):
            p = self._cvo().getPosition()
            return p.x, p.y
        def __set__(self, (double, double) value):
            cdef Point p
            p.x, p.y = value[0], value[1]
            self._cvo().setPosition(p)

    property direction:
        def __get__(self): return self._cvo().getDirection()
        def __set__(self, double value): self._cvo().setDirection(value)

    property speed:
        def __get__(self): return self._cvo().getSpeed()
        def __set__(self, double value): self._cvo().setSpeed(value)

    property edge_behavior:
        def __get__(self): return self._cvo().getEdgeBehavior()
        def __set__(self, value): self._cvo().setEdgeBehavior(value)

    def print_status(self):
        print(
            f"Sprite '{self.ID}' (visible: {self.visible})\n"
            f"\tat x={self.position[0]}, y={self.position[1]}\n"
            f"\twith speed {self.speed}, direction {self.direction}\n"
            f"\thas EdgeBehavior {self.edge_behavior}"
        )



cdef class PySprite(PyCanvasObject):
    # cdef Sprite* c_spr
    # cdef PySprite from_ptr(Sprite*, bool owner=*)

    def __cinit__(self, str fname):
        if fname == "":
            self._is_initialized = False
            return
        self._is_initialized = True
        self._ptr_owner = True
        self.c_spr = new Sprite(pystr_to_chars(fname))

    def __init__(self, *args, **kwargs):
        pass

    def __dealloc__(self):
        if self._ptr_owner:
            del self.c_spr

    cdef CanvasObject* _cvo(self):
        return self.c_spr

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

    property width:
        def __set__(self, int value): self.c_spr.setWidth(value)

    property height:
        def __set__(self, int value): self.c_spr.setHeight(value)

    def color(self, int x, int y):
        px = self.c_spr.getPixel(x, y)
        return px.red, px.green, px.blue


cdef class PyText(PyCanvasObject):
    # cdef Text* c_txt
    # cdef PyText from_ptr(Text*, bool owner=*)

    def __cinit__(self, str text):
        if text == "":
            self._is_initialized = False
            return
        fname = "lib/rgbmatrix/fonts/10x20.bdf"
        self.c_txt = new Text(pystr_to_chars(fname), pystr_to_chars(text))
        self._is_initialized = True
        self._ptr_owner = True

    def __init__(self, *args, **kwargs):
        pass

    def __dealloc__(self):
        if self._ptr_owner:
            del self.c_txt

    cdef CanvasObject* _cvo(self):
        return self.c_txt

    @staticmethod
    cdef PyText from_ptr(Text* text, bool owner=False):
        """
        See also:
        https://cython.readthedocs.io/en/latest/src/userguide/
        extension_types.html#existing-pointers-instantiation
        """
        cdef PyText py_text = PyText.__new__(PyText, "")
        py_text.c_txt = text
        py_text._is_initialized = True
        py_text._ptr_owner = owner
        return py_text
