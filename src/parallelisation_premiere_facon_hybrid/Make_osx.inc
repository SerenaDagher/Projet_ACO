CXX = /opt/homebrew/bin/g++-15

SDL_CFLAGS := $(shell /opt/homebrew/bin/sdl2-config --cflags)
SDL_LIBS   := $(shell /opt/homebrew/bin/sdl2-config --libs)

CXXFLAGS += $(SDL_CFLAGS) -fopenmp
CXXFLAGS2 += $(SDL_CFLAGS) -fopenmp

LIB = $(SDL_LIBS) -lpthread -fopenmp