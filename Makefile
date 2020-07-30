# CXXFLAGS: $(CXX) compiler flags
# CPPFLAGS: C Pre Processor path (e.g. for #include)
# LDFLAGS:	adds to ld's path-list
# LDLIBS:		-l$(name) searches ld's path-list for lib$(name).a

CYTHONIZE   ?=  cythonize
CYTHON      ?=  cython


CFLAGS			=		-Wall -Wextra -Wno-unused-parameter -O3 -g
CXXFLAGS		=		$(CFLAGS)
CPPFLAGS 		=		-Iinclude
# rpi-led-rgb-matrix (https://github.com/hzeller/rpi-rgb-led-matrix)
RGB_DIR     =   lib/rgbmatrix
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
CPPFLAGS    +=  -I/usr/include/python3.6 -I/usr/include/python3.7 -Ilib
#-I../matrix/bindings/python/rgbmatrix

include utility.mk 		# Functions run_and_test, sync_git

# Maybe todo: build a library out of the lib objects and only put the
# object file that belongs to the final binary in build/
# (see examples-api-use)
BINDINGS 		=		bindings/sprite.so bindings/panelwriter.so
BINDINGS_SRC=		lib/led-loop.cc lib/sprite.cc
OBJECTS			=		build/sprite.o build/led-loop.o
BINARIES		=		bin/shapeshifter


all : $(BINARIES) bindings

bindings : $(RGB_DIR) $(BINDINGS)


$(RGB_DIR)/lib/librgbmatrix.a: FORCE
	@$(call print_blue,Making rgbmatrix lib)
	$(MAKE) --no-print-directory -C $(RGB_DIR)/lib
	@$(call print_blue,Made rgbmatrix lib)

$(BINARIES): bin/% : $(RGB_DIR) $(OBJECTS) build/%.o
	@mkdir -p $(@D)
	@$(call run_and_test \
			,$(CXX) $(CXXFLAGS) $(CPPFLAGS) \
			 $(OBJECTS) build/$(@F).o -o $@ $(LDFLAGS) $(LDLIBS))

build/%.o : lib/%.cc
	@mkdir -p $(@D)
	@$(call run_and_test \
			,$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<)

bindings/%.cythonize.so : bindings/%.pyx
	@mkdir -p $(@D)
	@$(call run_and_test \
			,$(CYTHONIZE) $(CPPFLAGS) -X language_level=3 --inplace $@\
			,Cythonizing)

bindings/%.so : build/%.cython.cc
	@mkdir -p $(@D)
	@$(call run_and_test \
			,$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS))

build/%.cython.cc : bindings/%.pyx
	@mkdir -p $(@D)
	@$(call run_and_test \
			,$(CYTHON) $(CPPFLAGS) -X language_level=3 --cplus -o $@ $^ \
			,Building)

sync-to-pi :
	@$(call sync_git,"dietpi@192.168.178.36:/home/dietpi/shapeshifter/")

clean:
	rm -f $(OBJECTS) $(BINARIES) $(BINDINGS)
	rm -rf bin
	rm -rf build
	rm -f include/*.h.gch
	rm -rf bindings/__pycache__
	rm -rf bindings/*.log
	rm -rf bindings/*.so
	rm -rf bindings/*.cython.cc

FORCE:

.PHONY: FORCE sync-to-pi clean bindings
