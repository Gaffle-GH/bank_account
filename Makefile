CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -I include

BIN_DIR = bin
TARGET = $(BIN_DIR)/account

SRC_DIR = src
SRCS = $(SRC_DIR)/account.cc

$(TARGET): $(SRCS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: clean