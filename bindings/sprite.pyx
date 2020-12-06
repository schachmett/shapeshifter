# distutils: language = c++
# distutils: sources = sprite.cc, /usr/include/GraphicsMagick/Magick++.h
# cython: language_level=3

from libcpp cimport bool
from cython.operator cimport dereference as deref
from cython.operator cimport address

# from .sprite cimport Point, Sprite, CanvasObjectList, EdgeBehavior
from .utility cimport pystr_to_chars, cstr_to_pystr
from .magick cimport InitializeMagick


# This is super important: If it is not called, executing any magick++ code
# will lead to a segfault. Now it is called when importing this module
# (aka shapeshifter.so)
InitializeMagick(NULL)


cdef class PyCanvasObjectList:
    # cdef CanvasObjectList c_sprl
    # cdef py_sprites

    def __cinit__(self, **dict_of_pysprites):
        self.py_sprites = {}

    def __init__(self, **dict_of_pysprites):
        for key, py_sprite in dict_of_pysprites.items():
            self.__setitem__(key, py_sprite)

    def __getitem__(self, str key):
        py_sprite = self.py_sprites[key]
        return py_sprite

    def __setitem__(self, str key, PyCanvasObject cv_obj):
        if self.c_sprl.find(pystr_to_chars(key)) != self.c_sprl.end():
            print("overwriting")
        cv_obj.ID = key
        cdef CanvasObject* c_cvo = cv_obj._wrapped_CVO()
        self.c_sprl[pystr_to_chars(key)] = c_cvo
        # cdef Sprite c_spr = cv_obj._wrapped;
        # cdef Text c_txt;
        # if isinstance(cv_obj, PySprite):
        #     c_spr = cv_obj._wrapped()
        #     c_spr.setID(pystr_to_chars(key))
        #     self.c_sprl[pystr_to_chars(key)] = &c_spr
        # elif isinstance(cv_obj, PyText):
        #     c_txt = cv_obj._wrapped()
        #     c_txt.setID(pystr_to_chars(key))
        #     self.c_sprl[pystr_to_chars(key)] = &c_txt
        # else:
        #     raise NotImplementedError
        self.py_sprites[key] = cv_obj
        # cdef CanvasObject c_cvo = cv_obj._wrapped_CVO()
        # if c_cvo.getID() != pystr_to_chars(key):
        #     c_cvo.setID(pystr_to_chars(key))
        # self.c_sprl[pystr_to_chars(key)] = &c_cvo
        # self.py_sprites[key] = cv_obj

        # if isinstance(cv_obj, PySprite):
        #     cdef PySprite py_sprite = _PyCVO_to_PySprite(cv_obj)
        #     if py_sprite.c_spr.getID() != pystr_to_chars(key):
        #         py_sprite.c_spr.setID(pystr_to_chars(key))
        #         # print(f"Set ID {cstr_to_pystr(sprite.c_spr.getID())} to {key}")
        #     # cdef Sprite* c_spr = new Sprite(_str_to_chars(sprite.fname))
        #     self.c_sprl[pystr_to_chars(key)] = py_sprite.c_spr
        #     self.py_sprites[key] = py_sprite
        # elif isinstance(cv_obj, PyText):
        #     cdef PyText py_text = _PyCVO_to_PySprite(cv_obj)
        #     if py_text.c_txt.getID() != pystr_to_chars(key):
        #         py_text.c_txt.setID(pystr_to_chars(key))
        #     self.c_sprl[pystr_to_chars(key)] = py_text.c_txt
        #     self.py_sprites[key] = py_text
        # else:
        #     raise NotImplementedError

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


cdef class PyCanvasObject:
    def __cinit__(self, *args, **kwargs):
        self._is_initialized = False
        self._ptr_owner = True

    def __init__(self, *args, **kwargs):
        raise NotImplementedError("Should be abstract")

    def __dealloc__(self):
        if self._ptr_owner:
            del self.c_cvo

    cdef CanvasObject* _wrapped_CVO(self):
        return self.c_cvo

    property position:
        def __get__(self):
            p = deref(self._wrapped_CVO()).getPosition()
            return p.x, p.y
        def __set__(self, (double, double) value):
            cdef Point p
            p.x, p.y = value[0], value[1]
            self._wrapped_CVO().setPosition(p)



cdef class PySprite(PyCanvasObject):
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

    cdef Sprite* _wrapped(self):
        return self.c_spr

    cdef CanvasObject* _wrapped_CVO(self):
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

    def do_step(self):
        self._wrapped().doStep()

    property ID:
        def __get__(self): return self._wrapped().getID().decode("UTF-8")
        def __set__(self, str value): self._wrapped().setID(pystr_to_chars(value))

    property fname:
        def __get__(self): return self._wrapped().getSource()
        def __set__(self, str value): self._wrapped().setSource(value)

    property visible:
        def __get__(self): return self._wrapped().getVisible()
        def __set__(self, bool value): self._wrapped().setVisible(value)

    property width:
        def __get__(self): return self._wrapped().getWidth()
        def __set__(self, int value): self._wrapped().setWidth(value)

    property height:
        def __get__(self): return self._wrapped().getHeight()
        def __set__(self, int value): self._wrapped().setHeight(value)

    # property position:
    #     def __get__(self):
    #         p = self._wrapped().getPosition()
    #         return p.x, p.y
    #     def __set__(self, (double, double) value):
    #         cdef Point p
    #         p.x, p.y = value[0], value[1]
    #         self.c_spr.setPosition(p)
    #         # self._wrapped().setPosition(p)

    property direction:
        def __get__(self): return self._wrapped().getDirection()
        def __set__(self, double value): self._wrapped().setDirection(value)

    property speed:
        def __get__(self): return self._wrapped().getSpeed()
        def __set__(self, double value): self._wrapped().setSpeed(value)

    property edge_behavior:
        def __get__(self): return self._wrapped().getEdgeBehavior()
        def __set__(self, value): self._wrapped().setEdgeBehavior(value)

    def color(self, int x, int y):
        px = self._wrapped().getPixel(x, y)
        return px.red, px.green, px.blue

    def print_status(self):
        print(
            f"Sprite '{self.ID}' (visible: {self.visible})\n"
            f"\tat x={self.position[0]}, y={self.position[1]}\n"
            f"\twith speed {self.speed}, direction {self.direction}\n"
            f"\thas EdgeBehavior {self.edge_behavior}"
        )


cdef class PyText(PyCanvasObject):
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

    cdef Text _wrapped(self):
        return deref(self.c_txt)

    cdef CanvasObject* _wrapped_CVO(self):
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

    def do_step(self):
        self._wrapped().doStep()

    property ID:
        def __get__(self): return self._wrapped().getID().decode("UTF-8")
        def __set__(self, str value): self._wrapped().setID(pystr_to_chars(value))

    property fname:
        def __get__(self): return self._wrapped().getSource()
        def __set__(self, str value): self._wrapped().setSource(value)

    property visible:
        def __get__(self): return self._wrapped().getVisible()
        def __set__(self, bool value): self._wrapped().setVisible(value)

    property width:
        def __get__(self): return self._wrapped().getWidth()
        # def __set__(self, int value): self._wrapped().setWidth(value)

    property height:
        def __get__(self): return self._wrapped().getHeight()
        # def __set__(self, int value): self._wrapped().setHeight(value)

    # property position:
    #     def __get__(self):
    #         p = self._wrapped().getPosition()
    #         return p.x, p.y
    #     def __set__(self, (double, double) value):
    #         cdef Point p
    #         p.x, p.y = value[0], value[1]
    #         self._wrapped().setPosition(p)

    property direction:
        def __get__(self): return self._wrapped().getDirection()
        def __set__(self, double value): self._wrapped().setDirection(value)

    property speed:
        def __get__(self): return self._wrapped().getSpeed()
        def __set__(self, double value): self._wrapped().setSpeed(value)

    property edge_behavior:
        def __get__(self): return self._wrapped().getEdgeBehavior()
        def __set__(self, value): self._wrapped().setEdgeBehavior(value)

    def print_status(self):
        print(
            f"Sprite '{self.ID}' (visible: {self.visible})\n"
            f"\tat x={self.position[0]}, y={self.position[1]}\n"
            f"\twith speed {self.speed}, direction {self.direction}\n"
            f"\thas EdgeBehavior {self.edge_behavior}"
        )
