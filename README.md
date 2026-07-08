# <p align="center"> Bank Account Management  🧮</p>

## Description
**C++** Project that demonstrates Banking Account Management. 
<br/>This project covers many topics, such as: **File I/O**, **Classes**, and **Vectors**
<br>

###

## Makefile
### Requirements:
- **Compiler**: g++ or clang++ (`macOS typically includes clang++`)
- **GUI (optional)**: [FLTK](https://www.fltk.org/) — on macOS: `brew install fltk`
- **Windows users**: Install [WSL](https://learn.microsoft.com/en-us/windows/wsl/install) for access to both compilers, or [MinGW](https://www.mingw-w64.org/) for g++

### Build / Run / Clean:

**Release packages** (Mac + Windows zip files for GitHub Releases):

```bash
# macOS (build locally)
make all
bash scripts/package-release.sh macos v1.0.0
# -> dist/bank-account-macos-v1.0.0.zip

# Windows (MSYS2 MinGW shell)
export FLTK_CONFIG=/mingw64/bin/fltk-config
make all RELEASE=1
powershell -File scripts/package-release.ps1 windows v1.0.0
# -> dist/bank-account-windows-v1.0.0.zip
```

**Automated GitHub Releases:** push a version tag (e.g. `v1.0.0`) or run the **Build Release** workflow manually. It builds on `macos-latest` and `windows-latest`, then uploads both zip files to a GitHub Release when a `v*` tag is pushed.

| Platform | GUI | Notes |
|----------|-----|-------|
| **macOS** | `account_gui` (WebKit — same as web UI) | Recommended |
| **Windows** | `account_gui.exe` (FLTK) | Receipt-style native window |
| **Both** | `account_web` / `account` | Browser UI or terminal |

Each zip includes `web/` (required for GUI and web server). Extract and run from that folder.

---

**Native GUI** (recommended — exact HTML/CSS look in a desktop window):

```bash
make gui
make run
```

Opens `bin/account_gui`, a standalone macOS window that embeds the same `web/` UI you see in the browser (pixel-identical). Edit `web/index.html` and `web/styles.css` to change the look.

**Web UI** (same design in your browser):

```bash
make web
make run-web
```

**Terminal version** (original menu-driven app):

```bash
make cli
make run-cli
```

**FLTK GUI** (optional native window):

```bash
make gui
make run-gui
```

Build CLI + web:

```bash
make
```

```bash
make clean
```

---

## GUI version — how C++ GUIs work

This project uses **FLTK** (Fast Light Toolkit), a small cross-platform C++ GUI library. The GUI code lives in `src/gui_main.cc`; your banking logic stays in `src/account.cc` and is shared with the terminal app.

### The big idea: separate logic from the interface

Your `account` class exposes a **core API** that does not use `cin` or `cout`:

| Method | Purpose |
|--------|---------|
| `deposit(amount)` | Add money, append history |
| `withdraw(amount, errorMsg)` | Subtract money or return an error |
| `saveFile()` / `loadFile()` | Persist to `name_accountINFOCARD.txt` |
| `getBalance()`, `getHistory()` | Read state for the UI |

The terminal app (`src/main.cc`) calls `display()` — a text menu loop. The GUI (`src/gui_main.cc`) calls the same core methods when buttons are clicked. **One class, two interfaces.**

### FLTK building blocks

1. **Window** — `Fl_Window` is the root container. You position child widgets with `(x, y, width, height)` from the top-left corner.

2. **Widgets** — `Fl_Button`, `Fl_Input`, `Fl_Box`, `Fl_Hold_Browser` are the controls users see.

3. **Callbacks** — When a button is clicked, FLTK calls your function:
   ```cpp
   button->callback(onDeposit, &appState);
   ```
   The second argument is `user_data`: any pointer you want (here, our `BankApp` struct).

4. **Event loop** — `Fl::run()` waits for mouse clicks, key presses, and redraws. Your program does not loop with `while(true)`; FLTK calls your callbacks when things happen.

### Walk through `gui_main.cc`

1. **Login window** — User enters an account name. We probe `loadFile()` to see if a save file exists.
2. **Main window** — Shows balance, amount field, Deposit/Withdraw/Save/Quit buttons, and a history list.
3. **`refreshDisplay()`** — After every change, copy data from `account` into `Fl_Box` and `Fl_Hold_Browser` labels.
4. **`onDeposit` / `onWithdraw`** — Read the amount from `Fl_Input`, call `account::deposit` or `account::withdraw`, then refresh.

### Try these exercises

1. Change the window size in `buildMainWindow` and move widgets to match.
2. Add a **Clear amount** button that sets `amountInput` to `""`.
3. Disable the Withdraw button when balance is `$0.00` (hint: `withdrawBtn->deactivate()` / `activate()`).
4. Show a confirmation dialog before Quit if there are unsaved changes (compare file on disk vs in memory).

### Other C++ GUI options

| Library | Best for |
|---------|----------|
| **FLTK** (this project) | Learning, small apps, fast builds |
| **Qt** | Large desktop apps, rich widgets |
| **wxWidgets** | Native-looking cross-platform apps |
| **Dear ImGui** | Tools, debug UIs, games (immediate-mode style) |


