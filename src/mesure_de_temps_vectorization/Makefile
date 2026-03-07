CXX = /opt/homebrew/bin/g++-15

SDL_CFLAGS := $(shell /opt/homebrew/bin/sdl2-config --cflags)
SDL_LIBS   := $(shell /opt/homebrew/bin/sdl2-config --libs)

CPPFLAGS = -I/opt/homebrew/include -D_THREAD_SAFE
CXXFLAGS = -std=c++17 -fopenmp

ifdef DEBUG
CXXFLAGS += -g -O0 -Wall -fbounds-check -pedantic -D_GLIBCXX_DEBUG
CXXFLAGS2 = $(CXXFLAGS)
else
CXXFLAGS2 = $(CXXFLAGS) -O2 -march=native -Wall
CXXFLAGS  += -O3 -march=native -Wall
endif

LIB = $(SDL_LIBS) -lpthread -fopenmp

ALL = ant_simu.exe

default: help

all: $(ALL)

clean:
	@rm -fr *.o *.exe *~

.cpp.o:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS2) -c $< -o $@

ant_simu.exe: ant.o fractal_land.o renderer.o window.o ant_simu.o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS2) $^ -o $@ $(LIB)

help:
	@echo "Available targets : "
	@echo "    all            : compile all executables"
	@echo "Add DEBUG=yes to compile in debug"
	@echo "Configuration :"
	@echo "    CXX      :    $(CXX)"
	@echo "    CPPFLAGS :    $(CPPFLAGS)"
	@echo "    CXXFLAGS :    $(CXXFLAGS)"
	@echo "    LIB      :    $(LIB)"

%.html: %.md
	pandoc -s --toc $< --css=./github-pandoc.css --metadata pagetitle="OS202 - TD1" -o $@