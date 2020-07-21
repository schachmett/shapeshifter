CFLAGS=-Wall -Wextra -Wno-unused-parameter -O3 -g
CXXFLAGS=$(CFLAGS)

CYTHONIZE ?= cythonize
CYTHON ?= cython
CYTHON_CXXFLAGS=-shared -pthread -fPIC -Wall -O2 -fno-strict-aliasing

# Where our library resides. You mostly only need to change the
# RGB_LIB_DISTRIBUTION, this is where the library is checked out.
RGB_LIB_DISTRIBUTION=matrix
RGB_INCDIR=$(RGB_LIB_DISTRIBUTION)/include
RGB_LIBDIR=$(RGB_LIB_DISTRIBUTION)/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a
LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) -lrt -lm -lpthread

MAGICK_CXXFLAGS?=$(shell GraphicsMagick++-config --cppflags --cxxflags)
MAGICK_LDFLAGS?=$(shell GraphicsMagick++-config --ldflags --libs)

RAPIDJSON_INCDIR=rapidjson/include


OBJECTS=sprite.o shapeshifter.o
BINARIES=shapeshifter


all : $(BINARIES)

$(RGB_LIBRARY): FORCE
	$(MAKE) -C $(RGB_LIBDIR)

# $ sudo apt install libgraphicsmagick++-dev libwebp-dev
shapeshifter: $(OBJECTS) $(RGB_LIBRARY)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDFLAGS) $(MAGICK_LDFLAGS)

%.o : %.cc
	$(CXX) -I$(RGB_INCDIR) -I$(RAPIDJSON_INCDIR) \
			$(CXXFLAGS) $(MAGICK_CXXFLAGS) \
			-c -o $@ $<

%.cythonize.so : %.pyx
	$(CYTHONIZE) -X language_level=3 --inplace $@

%.so : %.cython.cc
	$(CXX) $(CYTHON_CXXFLAGS) -I/usr/include/python3.6 -o $@ $^

%.cython.cc : %.pyx
	$(CYTHON) -X language_level=3 --cplus -o $@ $^

sync-to-pi :
	rsync -auvhz -e ssh *.cc *.h *.py Makefile \
		dietpi@192.168.178.36:/home/dietpi/shapeshifter/

clean:
	rm -f $(OBJECTS) $(BINARIES)
	rm -f *.h.gch
	rm -rf build
	rm -rf *.so
	rm -rf *.cython.cc

FORCE:

.PHONY: FORCE sync-to-pi copy clean
