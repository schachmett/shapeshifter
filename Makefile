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

OUTDIR			=		shapeshifter
BINDINGS 		=		$(OUTDIR)/sprite.so $(OUTDIR)/panel.so $(OUTDIR)/loop.so
OBJECTS			=		build/sprite.o build/led-loop.o build/shapeshifter.o
BINARIES		=		$(OUTDIR)/shapeshifter


all : $(BINARIES) bindings

bindings : $(BINDINGS)
	@cp bindings/__init__.py $(OUTDIR)/


$(RGB_LIBRARY): FORCE
	@echo "###### Make matrix lib"
	$(MAKE) -C $(RGB_LIBDIR)
	@echo "######"

$(OUTDIR)/shapeshifter: $(OBJECTS) $(RGB_LIBRARY)
	@mkdir -p $(@D)
	@$(call run_and_test \
		,$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OBJECTS) -o $@ $(LDFLAGS) $(LDLIBS))

build/%.o : lib/%.cc
	@mkdir -p $(@D)
	@$(call run_and_test \
		,$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<)

$(OUTDIR)/%.cythonize.so : bindings/%.pyx
	@mkdir -p $(@D)
	@$(call run_and_test \
		,$(CYTHONIZE) $(CPPFLAGS) -X language_level=3 --inplace $@\
		,Cythonizing)

$(OUTDIR)/%.so : build/%.cython.cc
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
	rm -rf $(OUTDIR)
	rm -rf build
	rm -f include/*.h.gch
	rm -rf bindings/__pycache__
	rm -rf bindings/*.so
	rm -rf bindings/*.cython.cc

FORCE:

.PHONY: FORCE sync-to-pi clean bindings
