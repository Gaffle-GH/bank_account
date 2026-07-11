# <p align="center"> Bank Account Management</p>

## Description
**C++** Project that demonstrates Banking Account Management. 
<br/>This project covers many topics, such as: **File I/O**, **Classes**, and **Vectors**
<br>

###

## Makefile
### Requirements:
- **Compiler**: g++ or clang++ (`macOS typically includes clang++`)
- **macOS**: Xcode command-line tools (Cocoa + WebKit)
- **Windows**: [MSYS2 UCRT64](https://www.msys2.org/) with `mingw-w64-ucrt-x86_64-gcc`, plus the WebView2 runtime (usually preinstalled on Windows 11)

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
powershell -ExecutionPolicy Bypass -File scripts/fetch-webview2.ps1
make app RELEASE=1
powershell -File scripts/package-release.ps1 windows v1.0.0
# -> dist/bank-account-windows-v1.0.0.zip  (BankAccount.exe + WebView2Loader.dll)
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
| **Windows** | `BankAccount.exe` | WebView2 window (embedded HTML/CSS) |

Both platforms use the **same embedded web UI** (`web/index.html`, `web/styles.css`,
`web/app.js`) inside a native window:

| Platform | Shell | Native bridge features |
|----------|-------|------------------------|
| **macOS** | WebKit (`src/gui_webview.mm`) | Resize, folder picker, Finder open |
| **Windows** | WebView2 (`src/gui_webview_win.cc`) | Resize, folder picker, Explorer open |

Both open as a normal windowed app — **no terminal/console window appears**.
On macOS the release is packaged as a double-clickable `.app` bundle; on Windows
ship `BankAccount.exe` with `WebView2Loader.dll` in the same folder.

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

## GUI version — how it works

The default `make app` build embeds the web UI into a native desktop window on
both macOS and Windows. Banking logic stays in `src/account.cc` and is shared
with the optional terminal app.

### The big idea: separate logic from the interface

Your `account` class exposes a **core API** that does not use `cin` or `cout`:

| Method | Purpose |
|--------|---------|
| `deposit(amount)` | Add money, append history |
| `withdraw(amount, errorMsg)` | Subtract money or return an error |
| `saveFile()` / `loadFile()` | Persist to `name_accountINFOCARD.txt` |
| `getBalance()`, `getHistory()` | Read state for the UI |

The terminal app (`src/main.cc`) calls `display()` — a text menu loop. The GUI
calls the same core methods through the embedded web UI and HTTP API in
`src/web_runtime.cc`. **One class, two interfaces.**

### Native shells

- **macOS** — `src/gui_webview.mm` hosts the UI in WebKit and handles native
  messages from `web/app.js` (window sizing, folder picker, Finder).
- **Windows** — `src/gui_webview_win.cc` hosts the same UI in WebView2 with the
  same native bridge (window sizing, folder picker, Explorer).

Edit files in `web/` and rebuild — they are re-embedded into the executable
automatically.

### Optional legacy FLTK GUI

Linux builds (and manual fallback) can still use the older FLTK UI in
`src/gui_main.cc`. It is not the default desktop app on macOS or Windows.


