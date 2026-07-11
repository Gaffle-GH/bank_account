# <p align="center"> Bank Account Management</p>

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

The project compiles to **one self-contained application** — a native desktop
window with the web UI (`web/index.html`, `web/styles.css`, `web/app.js`)
embedded directly into the executable. There are no extra files to ship
alongside it.

```bash
make          # build the single app -> bin/BankAccount
make run      # build and launch it
```

**Release packages** (a single-file Mac + Windows zip for GitHub Releases):

```bash
# macOS (build locally)
make app
bash scripts/package-release.sh macos v1.0.0
# -> dist/bank-account-macos-v1.0.0.zip  (contains just BankAccount)

# Windows (MSYS2 UCRT64 shell)
make app RELEASE=1
powershell -File scripts/package-release.ps1 windows v1.0.0
# -> dist/bank-account-windows-v1.0.0.zip  (contains just BankAccount.exe)
```

**MACOS File Compatibility**

Apple Compatibility Issues will happen because of the Apple Developer ID Signing.
The only way to bypass this is to quarantine the application, using the following line in the terminal.
```bash
xattr -dr com.apple.quarantine ~/path_to_file/BankAccount.app
```
Note: You must do this whenever there's a new update to the application, and the application is also open source, so if you're fearful of viruses or malware, you can check the source code here.

**Automated GitHub Releases:** push a version tag (e.g. `v1.0.0`) or run the
**Build Release** workflow manually. It builds on `macos-latest` and
`windows-latest`, then uploads both zip files to a GitHub Release when a `v*`
tag is pushed.

| Platform | Application | UI |
|----------|-------------|-----|
| **macOS** | `BankAccount.app` | WebKit window (embedded HTML/CSS) |
| **Windows** | `BankAccount.exe` | FLTK receipt-style window |

Both open as a normal windowed app — **no terminal/console window appears**.
On macOS the release is packaged as a double-clickable `.app` bundle; on Windows
the executable is built for the GUI subsystem (`-mwindows`).

On the login screen you can set the **Save/Load folder** for the account file
(`NAME_accountINFOCARD.txt`). Leave it blank to use your Home folder; `~` is
expanded, and the folder is created if it doesn't exist.

Editing the UI: change the files in `web/` and rebuild — they are re-embedded
into the executable automatically.

---

#### Optional extra front-ends

These are not part of the default build, but are still available:

```bash
make web && make run-web   # same UI served in your browser at 127.0.0.1:8765
make cli && make run-cli   # original terminal menu version
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


