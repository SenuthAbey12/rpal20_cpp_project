CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET = rpal20

all: $(TARGET)

$(TARGET): rpal20.cpp
	$(CXX) $(CXXFLAGS) rpal20.cpp -o $(TARGET)

clean:
	rm -f $(TARGET) *.o output.*
