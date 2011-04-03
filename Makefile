
CC = g++
CFLAGS = -Wall -g
LDFLAGS = -lGL

headers = $(wildcard *.h)
sources = $(wildcard *.cpp zlib/*.cpp png/*.cpp)
objects = $(sources:.cpp=.o)

SED = sed -e 's/error/\x1b[31;1merror\x1b[0m/g' -e 's/warning/\x1b[33;1mwarning\x1b[0m/g'

ifdef MINGW
CC = i386-mingw32msvc-g++
CFLAGS = -Wall 
LDFLAGS = -lSDL_IMAGE -LGL -static-libgcc
else
CFLAGS += -DLINUX
endif


ifdef DEBUG
CFLAGS += -g
else
CFLAGS += -O3
endif


all: $(objects)
	@echo -e '\033[34;1m[ Basecode Library Built ]\033[0m'

%.o: %.cpp $(headers)
	@echo $< $(CFLAGS)
	@$(CC) $(CFLAGS) -c $< -o $@ 2>&1 | $(SED)
	
.PHONY: clean all
clean:
	rm -f *~ $(objects)
