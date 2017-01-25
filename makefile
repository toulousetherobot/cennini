CC  = gcc
CXX = g++

# Let’s leave a place holder for additional include directories

INCLUDES = 

# Compilation options:
# -g for debugging info and -Wall enables all warnings

CFLAGS   = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

# Linking options:
# -g for debugging info

LDFLAGS = -g `pkg-config --cflags --libs MagickCore`

# List the libraries you need to link with in LDLIBS
# For example, use "-lm" for the math library

LDLIBS = 

main: main.o sobel.o

main.o: main.c sobel.h

sobel.o: sobel.c sobel.h

.PHONY: clean
clean:
	rm -f *.o a.out core main

.PHONY: all
all: clean main