# Compiler
CXX = g++
WINDRES = windres

# Directories
SRC_DIR = ./src
INC_DIR = ./inc
OBJ_DIR = ./obj
RES_DIR = ./res
DB_DIR  = ./db
BIN_DIR = .

# Flags
CXXFLAGS = -std=c++17 -fopenmp -I\Users\Grant\Documents\SQLite
WFLAGS = -Wall -Wpedantic
LDFLAGS = -lstdc++fs -lsqlite3

# Executable name
TARGET = app

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# Object files
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS)) $(OBJ_DIR)/resources.o

# Resource files
RES = $(wildcard $(RES_DIR)/*.rc)

# Default target
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJS)
	$(CXX) -o $(BIN_DIR)/$(TARGET) $(OBJS) $(CXXFLAGS) $(LDFLAGS) $(WFLAGS)

# Rule to build object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(WFLAGS) -c $< -o $@

# Rule to compile resources
$(OBJ_DIR)/resources.o: $(RES_DIR)/resources.rc $(RES_DIR)/icon.ico
	$(WINDRES) $(RES_DIR)/resources.rc -O coff -o $(OBJ_DIR)/resources.o

# Clean rule to remove generated files
clean:
	powershell -Command "Remove-Item -Force $(OBJ_DIR)\\*.o"
	powershell -Command "Remove-Item -Force $(BIN_DIR)\\*.exe"

reset_databases:
	powershell -Command "Remove-Item -Force $(DB_DIR)\\*.db"

.PHONY: all clean reset_databases
