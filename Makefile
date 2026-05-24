CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET = rpal20

ifeq ($(OS),Windows_NT)
RM = cmd /C "for %%F in ($1) do if exist %%F del /Q /F %%F"
else
RM = rm -f $1
endif

all: $(TARGET)

$(TARGET): rpal20.cpp
	$(CXX) $(CXXFLAGS) rpal20.cpp -o $(TARGET)

clean:
	$(call RM,$(TARGET) *.o output.*)
