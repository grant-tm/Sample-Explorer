#===============================================================================
# COMPILER CONFIG
#===============================================================================

# Compiler
CXX = g++
WINDRES = windres

# Flags
CXXFLAGS = -std=c++17 -fopenmp -I/Users/Grant/Documents/SQLite
WFLAGS = -Wall -Wpedantic
LDFLAGS = -lstdc++fs -lsqlite3

#===============================================================================
# FILES
#===============================================================================

# Directories
SRC_DIR = ./src
INC_DIR = ./inc
OBJ_DIR = ./obj
RES_DIR = ./res
DB_DIR  = ./db
TST_DIR = ./tests
BIN_DIR = .

# Executable name
TARGET = app

# Source filepaths
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# Object filepaths
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS)) \
	   $(patsubst $(RES_DIR)/%.rc, 	$(OBJ_DIR)/%.o, $(RES))	

# Resource filepaths
RES = $(wildcard $(RES_DIR)/*.rc)

# Test filenames
TSTS = $(notdir $(basename $(wildcard $(TST_DIR)/*.cpp)))

#===============================================================================
# BUILD RULES
#===============================================================================

# Default: compile target
all: $(TARGET)

# Compile the target
$(TARGET): $(OBJS)
	$(CXX) -o $(BIN_DIR)/$(TARGET) $(OBJS) $(CXXFLAGS) $(LDFLAGS) $(WFLAGS)
	
# Compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(WFLAGS) -c $< -o $@

# Compile resources
$(OBJ_DIR)/resources.o: $(RES_DIR)/resources.rc $(RES_DIR)/icon.ico
	$(WINDRES) $(RES_DIR)/resources.rc -O coff -o $(OBJ_DIR)/resources.o

# Compile tests
tests:
	$(CXX) $(TST_DIR)/$(TSTS).cpp -o $(TST_DIR)/$(TSTS) \
	$(filter-out ./obj/app.o, $(OBJS)) $(CXXFLAGS) $(LDFLAGS) $(WFLAGS)

#===============================================================================
# CLEANUP
#===============================================================================

# default clean action is clean_build
clean: clean_build

# clean_all: Delete all .o and .exe files
clean_all: 
	@powershell -Command "\
	if ((Read-Host 'Clean Build, Tests, and Databases? [Y/N]') -eq 'Y') { \
		& '$(MAKE)' clean_build clean_tests clean_db; \
	} else { \
		Write-Output '--- Clean cancelled. No files were removed.'; \
	};"

# clean_build: Delete .o and .exe files associated with .cpp files in .\src
clean_build:
	@powershell -Command "\
		Write-Output 'Cleaning Build:'; \
		Write-Output '--- Deleting .\obj\*.o...'; \
		Remove-Item -Force $(OBJ_DIR)/*.o; \
		Write-Output '--- Deleting .\*.exe...'; \
		Remove-Item -Force $(BIN_DIR)/*.exe;
	
# clean_tests: Delete .o and .exe files associated with .cpp files in .\tests
clean_tests:
	@powershell -Command "\
		Write-Output 'Cleaning Tests:';	\
		Write-Output '--- Deleting .\tests\*.o...';	\
		Remove-Item -Force $(TST_DIR)/*.o; \
		Write-Output '--- Deleting .\tests\*.exe...'; \
		Remove-Item -Force $(TST_DIR)/*.exe;"

# clean_databsaes: Delele all .db databases
clean_db:
	@powershell -Command "\
	Write-Output 'Cleaning Databases:'; \
	if ((Read-Host '--- Type \"Confirm\" to delete databases') -eq 'Confirm') {\
		Write-Output '--- Deleting $(BIN_DIR)/*.db'; \
		Remove-Item -Force $(BIN_DIR)/*.db; \
	} else { \
		Write-Host 'Database deletion cancelled.'; \
	}"

#===============================================================================
# FLAGS
#===============================================================================

.PHONY: all clean reset_databases clean_all clean_build clean_tests clean_db tests
