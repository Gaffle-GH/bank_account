CXX ?= g++
CXXFLAGS = -Wall -Wextra -std=c++17 -I include

BIN_DIR = bin
EXE_EXT :=
ifeq ($(OS),Windows_NT)
EXE_EXT := .exe
endif

# The single, self-contained application (native GUI with the web UI embedded).
APP_TARGET = $(BIN_DIR)/BankAccount$(EXE_EXT)

# Optional extra front-ends (not built by default).
CLI_TARGET = $(BIN_DIR)/account$(EXE_EXT)
WEB_TARGET = $(BIN_DIR)/account_web$(EXE_EXT)

CORE_SRC = src/account.cc
RUNTIME_SRC = src/web_runtime.cc
CLI_SRC = src/main.cc
WEB_SRC = src/web_server.cc

# Web assets are embedded into the executable at build time.
EMBED_TOOL = $(BIN_DIR)/embed_assets$(EXE_EXT)
EMBED_SRC = tools/embed_assets.cc
GEN_SRC = src/web_assets.generated.cc
WEB_ASSETS = web/index.html web/styles.css web/app.js

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
else ifeq ($(OS),Windows_NT)
WEBVIEW2_PKG := $(firstword $(wildcard deps/Microsoft.Web.WebView2.*))
ifeq ($(WEBVIEW2_PKG),)
WEBVIEW2_PKG = deps/Microsoft.Web.WebView2
endif
GUI_SRC = src/gui_webview_win.cc
GUI_EXTRA_FLAGS = -I$(WEBVIEW2_PKG)/build/native/include
GUI_EXTRA_LDFLAGS = -lws2_32 -mwindows -municode -lole32 -lshell32 -lshlwapi -luuid -luser32 -lgdi32
else
GUI_SRC = src/gui_main.cc
GUI_EXTRA_FLAGS = $(FLTK_CXXFLAGS)
GUI_EXTRA_LDFLAGS = $(FLTK_LDFLAGS)
endif

ifeq ($(OS),Windows_NT)
ifeq ($(GUI_SRC),src/gui_webview_win.cc)
# GUI subsystem flags already set above.
else
GUI_EXTRA_LDFLAGS += -lws2_32 -mwindows
endif
WEB_LDFLAGS = -lws2_32
else
WEB_LDFLAGS =
endif

.PHONY: all app cli web package-macos package-windows run run-app run-cli run-web clean distclean

all: app

app: $(APP_TARGET)

cli: $(CLI_TARGET)

web: $(WEB_TARGET)

package-macos: app
	bash scripts/package-release.sh macos

package-windows: app
	powershell -ExecutionPolicy Bypass -File scripts/package-release.ps1 windows

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(EMBED_TOOL): $(EMBED_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(EMBED_TOOL) $(EMBED_SRC)

$(GEN_SRC): $(EMBED_TOOL) $(WEB_ASSETS)
	$(EMBED_TOOL) $(GEN_SRC) /index.html web/index.html /styles.css web/styles.css /app.js web/app.js

$(APP_TARGET): $(CORE_SRC) $(RUNTIME_SRC) $(GEN_SRC) $(GUI_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(GUI_EXTRA_FLAGS) -o $(APP_TARGET) $(CORE_SRC) $(RUNTIME_SRC) $(GEN_SRC) $(GUI_SRC) $(GUI_EXTRA_LDFLAGS)

ifeq ($(OS),Windows_NT)
ifeq ($(GUI_SRC),src/gui_webview_win.cc)
.PHONY: fetch-webview2
fetch-webview2:
	powershell -ExecutionPolicy Bypass -File scripts/fetch-webview2.ps1

$(APP_TARGET): fetch-webview2
endif
endif

$(CLI_TARGET): $(CORE_SRC) $(CLI_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(CLI_TARGET) $(CORE_SRC) $(CLI_SRC)

$(WEB_TARGET): $(CORE_SRC) $(RUNTIME_SRC) $(GEN_SRC) $(WEB_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(WEB_TARGET) $(CORE_SRC) $(RUNTIME_SRC) $(GEN_SRC) $(WEB_SRC) $(WEB_LDFLAGS)

run: run-app

run-app: $(APP_TARGET)
	./$(APP_TARGET)

run-cli: $(CLI_TARGET)
	./$(CLI_TARGET)

run-web: $(WEB_TARGET)
	./$(WEB_TARGET)

clean:
	rm -f $(APP_TARGET) $(CLI_TARGET) $(WEB_TARGET) $(EMBED_TOOL) $(GEN_SRC)

distclean: clean
	rm -rf dist
