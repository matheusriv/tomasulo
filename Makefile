CXX = g++

ifndef SYSTEMC_PATH
$(error SYSTEMC_PATH is not set. Please run 'export SYSTEMC_PATH=/path/to/systemc' before compiling)
endif

ifneq ($(wildcard $(SYSTEMC_PATH)/lib-linux64),)
    SYSTEMC_LIB_DIR ?= $(SYSTEMC_PATH)/lib-linux64
else ifneq ($(wildcard $(SYSTEMC_PATH)/lib-linux),)
    SYSTEMC_LIB_DIR ?= $(SYSTEMC_PATH)/lib-linux
else
    SYSTEMC_LIB_DIR ?= $(SYSTEMC_PATH)/lib
endif

CXXFLAGS = -std=c++17 -Wall -O2 -I$(SYSTEMC_PATH)/include -Isrc -DSC_ALLOW_DEPRECATED_IEEE_API
LDFLAGS = -L$(SYSTEMC_LIB_DIR) -lsystemc -lm -Wl,-rpath=$(SYSTEMC_LIB_DIR)
LDLIBPATH = LD_LIBRARY_PATH=$(SYSTEMC_LIB_DIR)

SRC_DIR = src
LOADER_DIR = src/loader
BUILD_DIR = build

# encontra automaticamente todos os arquivos .cpp
SRCS = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(LOADER_DIR)/*.cpp)

# mapeia os arquivos .cpp para arquivos .o equivalentes no diretório build
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(filter-out $(LOADER_DIR)/%.cpp, $(SRCS))) \
       $(patsubst $(LOADER_DIR)/%.cpp, $(BUILD_DIR)/loader/%.o, $(filter $(LOADER_DIR)/%.cpp, $(SRCS)))

TARGET = $(BUILD_DIR)/tomasulo

all: dir $(TARGET)

# cria as pastas de build necessárias
dir:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/loader

# linka os objetos gerando o executável final
$(TARGET): $(OBJS)
	@echo "Linking executable $@"
	@$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# compila os arquivos fonte da pasta src/ e src/loader/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	@echo "Running simulator..."
	@$(LDLIBPATH) ./$(TARGET) instrucoes.txt

clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)

.PHONY: all dir clean run