MPICXX = mpic++ -fopenmp

MPI_CPPFLAGS = 
MPI_CXXFLAGS = -std=c++17 -O3 -march=native -Wall
MPI_LIBS = -lmingw32 -lSDL2main -lSDL2

TARGET = ant_simu_mpi.exe

OBJS = ant.o fractal_land.o ant_simu_mpi.o

default: help

all: $(TARGET)

clean:
	@rm -f *.o *.exe *~

%.o: %.cpp
	$(MPICXX) $(MPI_CPPFLAGS) $(MPI_CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(MPICXX) $(MPI_CPPFLAGS) $(MPI_CXXFLAGS) $(OBJS) -o $(TARGET) $(MPI_LIBS)

help:
	@echo "Available targets:"
	@echo "  all    : compile the MPI version only"
	@echo "  clean  : remove object files and executable"
	@echo "Configuration:"
	@echo "  MPICXX       : $(MPICXX)"
	@echo "  MPI_CPPFLAGS : $(MPI_CPPFLAGS)"
	@echo "  MPI_CXXFLAGS : $(MPI_CXXFLAGS)"
	@echo "  MPI_LIBS     : $(MPI_LIBS)"