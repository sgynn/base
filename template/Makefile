exec = run
release = game
releasedir = release

OBJDIR = obj
CFLAGS =  -g -Wall -Isrc
LDFLAGS = -lbase -lGL -lX11 -lXxf86vm -lXcursor -lpthread
LDFLAGS += -logg -lvorbis -lvorbisfile -lopenal
#LDFLAGS += -lopenxr_loader
OSDEF = -DLINUX

headers = $(wildcard src/*.h src/*/*.h)
sources = $(wildcard src/*.cpp src/*/*.cpp)
objects = $(addprefix $(OBJDIR)/, $(sources:.cpp=.o))
dirs    = $(sort $(dir $(objects)))
release_objects= $(addprefix $(OBJDIR)/release/, $(sources:.cpp=.o))
release_dirs   = $(sort $(dir $(release_objects)))
data = $(wildcard data/*)
baselib = /usr/lib64/libbase.a

# Colour coding of g++ output - highlights errors and warnings
SED = sed -e 's/error/\x1b[31;1merror\x1b[0m/g' -e 's/warning/\x1b[33;1mwarning\x1b[0m/g'
SED2 = sed -e 's/undefined reference/\x1b[31;1mundefined reference\x1b[0m/g'

ifeq ($(OS),Windows_NT)
LDFLAGS = -lbase -lgdi32 -lopengl32 -static-libgcc -static-libstdc++ -lwinmm -static -lpthread
LDFLAGS += -logg -lvorbis -lvorbisfile.dll -lOpenAL32.dll
LDFLAGS += -lopenxr_loader
LDFLAGS += -mwindows
OBJDIR = obj/mingw
OSDEF = -DWIN32
exec := $(exec).exe
release := $(release).exe

ifneq ($(wildcard icon.ico), "")
objects += $(OBJDIR)/resources.res
release_objects += $(OBJDIR)/resources.res
endif

endif


################################## EMSCRIPTEN OPTIONS ##########################
EMOptions += -s FULL_ES3=1
EMOptions += -s LEGACY_GL_EMULATION=0
EMOptions += -s WASM=0
EMOptions += -s ERROR_ON_UNDEFINED_SYMBOLS=0
EMOptions += -s USE_FREETYPE=1
EMOptions += -s USE_OGG=1
EMOptions += -s USE_VORBIS=1
#EMOptions += -s USE_PTHREADS=1 -pthread
#EMOptions += -s PTHREAD_POOL_SIZE=3
#EMOptions += -s ALLOW_MEMORY_GROWTH=1
EMOptions += -s INITIAL_MEMORY=44761088
#EMOptions += -s SAFE_HEAP=1  # Use for debugging heap corruption errors

EM = /opt/emsdk/upstream/emscripten
EMSDK := /opt/emsdk
EM_CONFIG := /opt/emsdk/.emscripten
EMSDK_NODE = /opt/emsdk/node/14.15.5_64bit/bin/node
em_objects = $(addprefix $(OBJDIR)/em/, $(sources:.cpp=.o))
em_dirs   = $(sort $(dir $(em_objects)))
em_data = $(addprefix --preload-file , $(shell find -L data/ -type d))




CFLAGS += -DAUDIO

.PHONY: clean windows

all: $(exec)

$(exec): CFLAGS += -DDEBUG $(OSDEF)
$(exec): $(dirs) $(objects) $(baselib) Makefile
	@echo -e "\033[34;1m[ Linking ]\033[0m"
	@echo $(CXX) -o $(exec) $(objects) $(CFLAGS) $(LDFLAGS)
	@$(CXX) -o $(exec) $(objects) $(CFLAGS) $(LDFLAGS) 2>&1 | $(SED2)


$(release): CFLAGS += -O2 $(OSDEF)
$(release): $(release_dirs) $(release_objects) $(baselib) Makefile
	@echo -e "\033[34;1m[ Linking ]\033[0m"
	@echo $(CXX) -o $(release) $(release_objects) $(CFLAGS) $(LDFLAGS)
	@$(CXX) -o $(release) $(release_objects) $(CFLAGS) $(LDFLAGS) 2>&1 | $(SED2)


webgl/index.js: PATH := ${EM}:${PATH}
webgl/index.js: CFLAGS += -DEMSCRIPTEN
webgl/index.js: $(em_dirs) $(em_objects) $(data) Makefile
	em++ -o webgl/index.js -lbase $(em_objects) $(CFLAGS) $(EMOptions) $(em_data)
webgl/:
	mkdir webgl
	cp ~/projects/library/base/web-template.html webgl/index.html
	sed -i "s/\$$title/${releasedir}/" webgl/index.html




$(OBJDIR)/%.o: %.cpp $(headers)
	@echo $<
	@$(CXX) $(CFLAGS) -c $< -o $@ 2>&1 | $(SED)

$(OBJDIR)/release/%.o: %.cpp $(headers)
	@echo $<
	@$(CXX) $(CFLAGS) -c $< -o $@ 2>&1 | $(SED)

$(OBJDIR)/em/%.o: %.cpp $(headers)
	@echo $<
	@em++ $(CFLAGS) -c $< -o $@ 2>&1 | $(SED)

$(OBJDIR)/resources.res : icon.ico
	echo "APPICON ICON icon.ico" > $(OBJDIR)/res.rc
	cp icon.ico $(OBJDIR)
	x86_64-w64-mingw32-windres $(OBJDIR)/res.rc -O coff -o $(OBJDIR)/resources.res

$(OBJDIR)/release/%/: %
	@mkdir -p $@
$(OBJDIR)/em/%/: %
	@mkdir -p $@
$(OBJDIR)/%/: %
	@mkdir -p $@
$(OBJDIR)/:
	@mkdir -p $(OBJDIR)

clean:
	rm -f *~ */*~ $(exec) $(release).exe $(release)
	rm -rf $(OBJDIR)




windows:
	mingw64-make $(release).exe OS=Windows_NT -j8
	x86_64-w64-mingw32-strip $(release).exe

release: $(release) windows $(data)
	mkdir -p $(releasedir)
	rsync -rL data $(releasedir)/
	cp $(release) $(releasedir)/$(release).x86_64
	cp $(release).exe $(releasedir)/$(release).exe
	strip $(releasedir)/$(release).x86_64

$(releasedir).zip: $(releasedir)/* $(releasedir)/data/*
	zip $(releasedir).zip -ru $(releasedir)

upload: $(releasedir).zip
	scp $(releasedir).zip draknek.org:/home/samgynn/sam.draknek.org/files/

upload-web: webgl/index.js
	rsync -r webgl/ draknek.org:/home/samgynn/sam.draknek.org/files/$(releasedir)/

web: webgl/ webgl/index.js html5.zip

html5.zip: webgl/index.js
	@echo -e '\e[1;34mCreating zip file for ludum dare embedded upload\e[0m'
	@zip -j html5.zip  webgl/*



