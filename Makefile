# Compiler and flags
CXX = g++-15
CXXFLAGS = -std=c++17 -Wall -Wextra -g -Iinclude

# Source files
SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.cpp) server.cpp
# OBJS = $(SRCS:.cpp=.o)
OBJS = $(patsubst %.cpp, %.o, $(SRCS))


# Output
TARGET = server

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile each .cpp into .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean built files
clean:
	rm -f $(OBJS) $(TARGET)
