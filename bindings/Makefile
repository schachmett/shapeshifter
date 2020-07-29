CYTHONIZE   ?=  cythonize
CYTHON      ?=  cython

CFLAGS      =   -Wall -Wextra -Wno-unused-parameter -O3 -g
CXXFLAGS    =   $(CFLAGS)
# rpi-led-rgb-matrix (https://github.com/hzeller/rpi-rgb-led-matrix)
RGB_DIR     =   ../matrix
CPPFLAGS    +=  -I$(RGB_DIR)/include
LDFLAGS     +=  -L$(RGB_DIR)/lib
LDLIBS      +=  -lrgbmatrix -lrt -lm -lpthread
# Magick++ ($ sudo apt install libgraphicsmagick++-dev libwebp-dev)
CXXFLAGS    +=  $(shell GraphicsMagick++-config --cxxflags)
CPPFLAGS    +=  $(shell GraphicsMagick++-config --cppflags)
LDFLAGS     +=  $(shell GraphicsMagick++-config --ldflags)
LDLIBS      +=  $(shell GraphicsMagick++-config --libs)
# Cython ($ python3 -m pip install cython)
CXXFLAGS    +=  -shared -pthread -fPIC -Wall -O2 -fno-strict-aliasing
CPPFLAGS    +=  -I/usr/include/python3.6 -I/usr/include/python3.7
#-I../matrix/bindings/python/rgbmatrix

include ../*.mk

BINDINGS=sprite.so panel.so loop.so


all : $(BINDINGS)


bin/%.cythonize.so : bindings/%.pyx
	@$(call run_and_test \
		,$(CYTHONIZE) $(CPPFLAGS) -X language_level=3 --inplace $@\
		,Cythonizing $^ to)

bin/%.so : build/%.cython.cc
	@$(call run_and_test \
		,$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS) \
		,Compiling $^ to)

build/%.cython.cc : bindings/%.pyx
	@$(call run_and_test \
		,$(CYTHON) $(CPPFLAGS) -X language_level=3 --cplus -o $@ $^,\
		,Cythoning $^ to)

clean:
	rm -f $(OBJECTS) $(BINARIES)
	rm -f *.h.gch
	rm -rf build
	rm -rf *.so
	rm -rf *.cython.cc

FORCE:

.PHONY: FORCE clean
