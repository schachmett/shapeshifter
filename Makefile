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


OBJECTS=sprite.o led-loop.o shapeshifter.o
BINARIES=shapeshifter
BINDINGS=bindings.so


all : $(BINARIES)

bindings : $(BINDINGS)


$(RGB_LIBRARY): FORCE				# beware!
	$(MAKE) -C $(RGB_LIBDIR)

# $ sudo apt install libgraphicsmagick++-dev libwebp-dev
shapeshifter: $(OBJECTS) $(RGB_LIBRARY)
	@echo "###### Make shapeshifter"
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDFLAGS) $(MAGICK_LDFLAGS)
	@echo "######"

%.o : %.cc
	@echo "###### Make object file $@"
	$(CXX) $(CXXFLAGS) -I$(RGB_INCDIR) -I$(RAPIDJSON_INCDIR) $(MAGICK_CXXFLAGS) -c -o $@ $<
	@echo "######"

%.cythonize.so : %.pyx
	$(CYTHONIZE) -X language_level=3 --inplace $@

%.so : %.cython.cc
	$(CXX) $(CYTHON_CXXFLAGS) $(MAGICK_CXXFLAGS) -I/usr/include/python3.6 -o $@ $^ $(MAGICK_LDFLAGS)

%.cython.cc : %.pyx
	$(CYTHON) -X language_level=3 --cplus -o $@ $^

sync-to-pi :
	@echo "Syncing to Raspberry Pi"
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
