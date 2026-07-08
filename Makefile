CXX ?= g++
CXXFLAGS = -Wall -Wextra -std=c++17 -I include

BIN_DIR = bin
EXE_EXT :=
ifeq ($(OS),Windows_NT)
EXE_EXT := .exe
endif

CLI_TARGET = $(BIN_DIR)/account$(EXE_EXT)
GUI_TARGET = $(BIN_DIR)/account_gui$(EXE_EXT)
WEB_TARGET = $(BIN_DIR)/account_web$(EXE_EXT)

CORE_SRC = src/account.cc
RUNTIME_SRC = src/web_runtime.cc
CLI_SRC = src/main.cc
WEB_SRC = src/web_server.cc

UNAME_S := $(shell uname -s 2>/dev/null)

FLTK_CONFIG ?= fltk-config
ifeq ($(UNAME_S),Darwin)
FLTK_CONFIG := /opt/homebrew/opt/fltk/bin/fltk-config
endif

FLTK_CXXFLAGS := $(shell $(FLTK_CONFIG) --cxxflags 2>/dev/null)
ifeq ($(RELEASE),1)
FLTK_LDFLAGS := $(shell $(FLTK_CONFIG) --ldstaticflags 2>/dev/null)
else
FLTK_LDFLAGS := $(shell $(FLTK_CONFIG) --ldflags 2>/dev/null)
endif

ifeq ($(UNAME_S),Darwin)
GUI_SRC = src/gui_webview.mm
GUI_EXTRA_FLAGS =
GUI_EXTRA_LDFLAGS = -framework Cocoa -framework WebKit
else
GUI_SRC = src/gui_main.cc
GUI_EXTRA_FLAGS = $(FLTK_CXXFLAGS)
GUI_EXTRA_LDFLAGS = $(FLTK_LDFLAGS)
endif

ifeq ($(OS),Windows_NT)
GUI_EXTRA_LDFLAGS += -lws2_32
WEB_LDFLAGS = -lws2_32
else
WEB_LDFLAGS =
endif

.PHONY: all cli gui web package-macos package-windows run run-cli run-gui run-web clean distclean

all: cli gui web

cli: $(CLI_TARGET)

gui: $(GUI_TARGET)

web: $(WEB_TARGET)

package-macos: all
	bash scripts/package-release.sh macos

package-windows: all
	powershell -ExecutionPolicy Bypass -File scripts/package-release.ps1 windows

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(CLI_TARGET): $(CORE_SRC) $(CLI_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(CLI_TARGET) $(CORE_SRC) $(CLI_SRC)

$(GUI_TARGET): $(CORE_SRC) $(RUNTIME_SRC) $(GUI_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(GUI_EXTRA_FLAGS) -o $(GUI_TARGET) $(CORE_SRC) $(RUNTIME_SRC) $(GUI_SRC) $(GUI_EXTRA_LDFLAGS)

$(WEB_TARGET): $(CORE_SRC) $(RUNTIME_SRC) $(WEB_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(WEB_TARGET) $(CORE_SRC) $(RUNTIME_SRC) $(WEB_SRC) $(WEB_LDFLAGS)

run: run-gui

run-cli: $(CLI_TARGET)
	./$(CLI_TARGET)

run-gui: $(GUI_TARGET)
	./$(GUI_TARGET)

run-web: $(WEB_TARGET)
	./$(WEB_TARGET)

clean:
	rm -f $(CLI_TARGET) $(GUI_TARGET) $(WEB_TARGET)

distclean: clean
	rm -rf dist
