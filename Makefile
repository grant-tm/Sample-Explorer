# Compiler
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -fopenmp -Wall -Wpedantic -I\Users\Grant\Documents\SQLite

# Linker flags
LDFLAGS = -lstdc++fs -lsqlite3

# Executable name
TARGET = app

# Source files
SRCS = app.cpp \
       DatabaseInteractions.cpp \
       FileProcessing.cpp \
       SystemUtilities.cpp \
       UI.cpp \
       UIState.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Header files (for dependency checking)
HDRS = DatabaseInteractions.h \
       ExplorerFile.h \
       FileProcessing.h \
       SystemUtilities.h \
       ThreadSafeQueue.h \
       UI.h \
       UIState.h

# Default target
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

# Rule to build object files
%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule to remove generated files
clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean