CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2
OBJ = chip8.o main.o
TARGET = chip8

chip8: chip8.o main.o
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

chip8.o: chip8.cpp chip8.h
	$(CXX) $(CXXFLAGS) -c chip8.cpp

main.o: main.cpp chip8.h
	$(CXX) $(CXXFLAGS) -c main.cpp

clean:
	rm -f *.o $(TARGET)
