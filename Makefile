#===============================================================================
# COMPILER CONFIG
#===============================================================================

# Compiler
CXX = g++
WINDRES = windres

# Flags
CXXFLAGS = -std=c++17 -fopenmp -I/Users/Grant/Documents/SQLite
WFLAGS = -Wall -Wpedantic
LDFLAGS = -lstdc++fs -lsqlite3 -fopenmp

#===============================================================================
# FILES
#===============================================================================

# Build Directories
SRC_DIR = ./src
INC_DIR = ./inc
OBJ_DIR = ./obj
RES_DIR = ./res
DB_DIR  = ./db
BIN_DIR = .

# Build target, sources, objects, and resources
TGT = app
SRC = $(wildcard $(SRC_DIR)/*.cpp)
RES = $(wildcard $(RES_DIR)/*.rc)
OBJ = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC)) \
	   	  $(patsubst $(RES_DIR)/%.rc, 	$(OBJ_DIR)/%.o, $(RES))
OBJ := $(filter-out $(OBJ_DIR)/KeySignitureDetector.o, $(OBJ))

# Test Directories
TST_SRC_DIR = ./tests/src
TST_BIN_DIR = ./tests/bin
TST_OBJ_DIR = ./tests/obj

# Test sources, objects, and binaries
TST_SRC = $(wildcard $(TST_SRC_DIR)/*.cpp)
TST_OBJ = $(patsubst $(TST_SRC_DIR)/%.cpp, $(TST_OBJ_DIR)/%.o, $(TST_SRC))
TST_BIN = $(patsubst $(TST_SRC_DIR)/%.cpp, $(TST_BIN_DIR)/%, $(TST_SRC))

# Includes
SQLITE3   = ../repos/sqlite
AUDIOFILE = ../repos/AudioFile/
KISSFFT   = ../repos/kissfft/
KISSFFTC  = ../repos/kissfft/kiss_fft.c
INC = -I $(SQLITE3) -I $(AUDIOFILE) -I $(KISSFFT) -I ./inc

#===============================================================================
# BUILD RULES
#===============================================================================

# Default: compile target
all: $(TGT)

# compile the target
$(TGT): $(OBJ_DIR)/KeyDet.o $(OBJ) 
	$(CXX) -o $(BIN_DIR)/$(TGT) $(OBJ) $(OBJ_DIR)/KeyDet.o ../repos/kissfft/kiss_fft.o $(CXXFLAGS) $(LDFLAGS) $(WFLAGS) $(INC)

# comiple keydet
$(OBJ_DIR)/KeyDet.o: $(SRC_DIR)/KeySignitureDetector.cpp 
	$(CXX) -c $< -o $@ $(CXXFLAGS) $(WFLAGS) $(INC)

# compile objs
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS) $(WFLAGS) $(INC)

# compile resources
$(OBJ_DIR)/resources.o: $(RES_DIR)/resources.rc 
	$(WINDRES) $(RES_DIR)/resources.rc -O coff -o $(OBJ_DIR)/resources.o

# ensure object directory exists
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

#===============================================================================
# TEST BUILD RULES
#===============================================================================

# compile test objs
$(TST_OBJ_DIR)/%.o: $(TST_SRC_DIR)/%.cpp | $(TST_OBJ_DIR)
	$(CXX) -c $< -o $@ $(CXXFLAGS) $(WFLAGS)

# link test binaries
$(TST_BIN_DIR)/%: $(TST_OBJ_DIR)/%.o $(filter-out $(OBJ_DIR)/$(TGT).o, $(OBJ)) | $(TST_BIN_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(WFLAGS)

# ensure test object directory exists
$(TST_OBJ_DIR):
	mkdir -p $(TST_OBJ_DIR)

# ensure test binary directory exists
$(TST_BIN_DIR):
	mkdir -p $(TST_BIN_DIR)

# Test compilation rule
tests: $(TST_BIN)

run_test: tests
	@powershell -Command "Get-ChildItem -Path $(TST_BIN_DIR) -Filter *.exe | ForEach-Object { Start-Process -FilePath ($$_.FullName) -Wait }"

#===============================================================================
# CLEANUP SCRIPTS
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
		Write-Output '--- Deleting Executables ($(BIN_DIR)/*.exe)'; \
		Remove-Item -Force $(BIN_DIR)/*.exe; \
		Write-Output '--- Deleting Object Files ($(OBJ_DIR)/*.o)'; \
		Remove-Item -Force $(OBJ_DIR)/*.o;
	
# clean_tests: Delete .o and .exe files associated with .cpp files in .\tests
clean_tests:
	@powershell -Command " \
		Write-Output 'Cleaning Tests:';	\
		Write-Output '--- Deleting Executables ($(TST_BIN_DIR)/.exe)'; \
		Remove-Item -Force $(TST_BIN_DIR)/*.exe; \
		Remove-Item -Force $(TST_BIN_DIR)/*; \
		Write-Output '--- Deleting Object Files ($(TST_OBJ_DIR)/.o)'; \
		Remove-Item -Force $(TST_OBJ_DIR)/*.o; \
		

# clean_databsaes: Delele all .db databases
clean_db:
	@powershell -Command "\
	Write-Output 'Cleaning Databases:'; \
	if ((Read-Host '--- Type \"Confirm\" to delete databases') -eq 'Confirm') {\
		Write-Output '--- Deleting Databases ($(BIN_DIR)/*.db)'; \
		Remove-Item -Force $(BIN_DIR)/*.db; \
	} else { \
		Write-Host 'Aborted Database deletion.'; \
	}"

#===============================================================================
# FLAGS
#===============================================================================

.PHONY: 
	all 
	clean clean_all clean_build clean_tests clean_db 
	tests run_test
