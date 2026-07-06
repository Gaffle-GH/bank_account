CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -I include

FLTK_CONFIG ?= /opt/homebrew/opt/fltk/bin/fltk-config
FLTK_CXXFLAGS := $(shell $(FLTK_CONFIG) --cxxflags 2>/dev/null)
FLTK_LDFLAGS := $(shell $(FLTK_CONFIG) --ldflags 2>/dev/null)

BIN_DIR = bin
CLI_TARGET = $(BIN_DIR)/account
GUI_TARGET = $(BIN_DIR)/account_gui
WEB_TARGET = $(BIN_DIR)/account_web

CORE_SRC = src/account.cc
RUNTIME_SRC = src/web_runtime.cc
CLI_SRC = src/main.cc
WEB_SRC = src/web_server.cc

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
GUI_SRC = src/gui_webview.mm
GUI_EXTRA_FLAGS =
GUI_EXTRA_LDFLAGS = -framework Cocoa -framework WebKit
else
GUI_SRC = src/gui_main.cc
GUI_EXTRA_FLAGS = $(FLTK_CXXFLAGS)
GUI_EXTRA_LDFLAGS = $(FLTK_LDFLAGS)
endif

.PHONY: all cli gui web run run-cli run-gui run-web clean

all: cli gui

cli: $(CLI_TARGET)

gui: $(GUI_TARGET)

web: $(WEB_TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(CLI_TARGET): $(CORE_SRC) $(CLI_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(CLI_TARGET) $(CORE_SRC) $(CLI_SRC)

$(GUI_TARGET): $(CORE_SRC) $(RUNTIME_SRC) $(GUI_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(GUI_EXTRA_FLAGS) -o $(GUI_TARGET) $(CORE_SRC) $(RUNTIME_SRC) $(GUI_SRC) $(GUI_EXTRA_LDFLAGS)

$(WEB_TARGET): $(CORE_SRC) $(RUNTIME_SRC) $(WEB_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(WEB_TARGET) $(CORE_SRC) $(RUNTIME_SRC) $(WEB_SRC)

run: run-gui

run-cli: $(CLI_TARGET)
	./$(CLI_TARGET)

run-gui: $(GUI_TARGET)
	./$(GUI_TARGET)

run-web: $(WEB_TARGET)
	./$(WEB_TARGET)

clean:
	rm -f $(CLI_TARGET) $(GUI_TARGET) $(WEB_TARGET)
