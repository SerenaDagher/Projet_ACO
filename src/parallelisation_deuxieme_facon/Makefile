MPICXX = mpicxx

CPPFLAGS = -I/opt/homebrew/include -D_THREAD_SAFE
CXXFLAGS = -std=c++17 -O3 -march=native -Wall
LIBS = -L/opt/homebrew/lib -lSDL2

TARGET = ant_simu_mpi_domain.exe
OBJS = ant.o fractal_land.o ant_simu_mpi_domain.o

default: help

all: $(TARGET)

clean:
	@rm -f *.o *.exe *~

%.o: %.cpp
	$(MPICXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(MPICXX) $(CPPFLAGS) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(LIBS)

help:
	@echo "Targets:"
	@echo "  all   : compile la seconde façon MPI"
	@echo "  clean : supprime les fichiers objets"