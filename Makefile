CXX = clang++
CXXFLAGS = -std=c++14 -pthread
LDFLAGS = -pthread
SFML = -lsfml-graphics -lsfml-window -lsfml-system
LDFLAGS += $(SFML)

all: barbershop

barbershop: barbershop.cpp shapes.cpp shapes.h
	$(CXX) $(CXXFLAGS) barbershop.cpp shapes.cpp -o barbershop $(LDFLAGS)

clean:
	rm -f barbershop
