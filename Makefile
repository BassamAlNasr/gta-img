# Compiler and its flags.
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O1

# List of source files.
SOURCES = $(wildcard src/*.cpp)

# Object files.
OBJECTS = $(SOURCES:.cpp=.o)

# Output executable.
EXEC = gta-img

# Default target.
all: $(EXEC)

# Link the object files to create the executable.
$(EXEC): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(EXEC)

# Compile the .cpp files to .o files.
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up object files and the executable.
clean:
	rm -f $(OBJECTS) $(EXEC)

# Phony targets.
.PHONY: all clean
