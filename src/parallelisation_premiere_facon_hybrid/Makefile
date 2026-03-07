MPICXX = env OMPI_CXX=/opt/homebrew/bin/g++-15 mpicxx

CPPFLAGS = -I/opt/homebrew/include -D_THREAD_SAFE
CXXFLAGS = -std=c++17 -O3 -march=native -Wall -fopenmp
LIBS = -L/opt/homebrew/lib -lSDL2 -fopenmp

TARGET = ant_simu_mpi.exe
OBJS = ant.o fractal_land.o ant_simu_mpi.o

default: help

all: $(TARGET)

clean:
	rm -f *.o *.exe *~

%.o: %.cpp
	$(MPICXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(MPICXX) $(CPPFLAGS) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

help:
	@echo "Targets:"
	@echo "  all   : compile hybrid MPI + OpenMP version"
	@echo "  clean : remove objects and executable"
	@echo "Configuration:"
	@echo "  MPICXX   = $(MPICXX)"
	@echo "  CPPFLAGS = $(CPPFLAGS)"
	@echo "  CXXFLAGS = $(CXXFLAGS)"
	@echo "  LIBS     = $(LIBS)"