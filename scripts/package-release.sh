#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

PLATFORM="${1:?Usage: package-release.sh macos|windows [version]}"
VERSION="${2:-dev}"

STAGING="dist/bank-account-${PLATFORM}-${VERSION}"
ARCHIVE="dist/bank-account-${PLATFORM}-${VERSION}.zip"

mkdir -p dist
rm -rf "$STAGING"
mkdir -p "$STAGING"

if [[ "$PLATFORM" == "macos" ]]; then
  if [[ ! -f "bin/BankAccount" ]]; then
    echo "Missing bin/BankAccount — run 'make app' first" >&2
    exit 1
  fi

  # Build a proper .app bundle so double-clicking launches a windowed app
  # with no Terminal window.
  APP_BUNDLE="$STAGING/BankAccount.app"
  mkdir -p "$APP_BUNDLE/Contents/MacOS"
  cp "bin/BankAccount" "$APP_BUNDLE/Contents/MacOS/BankAccount"
  chmod +x "$APP_BUNDLE/Contents/MacOS/BankAccount"

  cat > "$APP_BUNDLE/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleName</key>
  <string>BankAccount</string>
  <key>CFBundleDisplayName</key>
  <string>Bank Account</string>
  <key>CFBundleIdentifier</key>
  <string>com.gaffle.bankaccount</string>
  <key>CFBundleVersion</key>
  <string>1.0</string>
  <key>CFBundleShortVersionString</key>
  <string>1.0</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleExecutable</key>
  <string>BankAccount</string>
  <key>LSMinimumSystemVersion</key>
  <string>10.13</string>
  <key>NSHighResolutionCapable</key>
  <true/>
</dict>
</plist>
PLIST

  cp LICENSE "$STAGING/"

  cat > "$STAGING/RUN.txt" <<'EOF'
Bank Account (macOS)
====================

Double-click "BankAccount.app" — it opens a window, no Terminal required.
If macOS blocks it the first time: right-click the app -> Open, then confirm.

The web UI is built into the app, so there are no extra files to keep with it.
Account data is saved into the folder you choose on the login screen
(blank = your Home folder), as NAME_accountINFOCARD.txt.
EOF

  # Ad-hoc sign so the app bundle has a code signature.
  # (Not notarized; this just prevents "unsigned bundle" issues.)
  if command -v codesign >/dev/null 2>&1; then
    codesign --force --deep --sign - "$APP_BUNDLE" >/dev/null 2>&1 || true
  fi

elif [[ "$PLATFORM" == "windows" ]]; then
  APP="BankAccount.exe"
  if [[ ! -f "bin/${APP}" ]]; then
    echo "Missing bin/${APP} — run 'make app' first" >&2
    exit 1
  fi
  cp "bin/${APP}" "$STAGING/"
  cp LICENSE "$STAGING/"

  cat > "$STAGING/RUN.txt" <<'EOF'
Bank Account (Windows)
======================

Double-click "BankAccount.exe" — it opens a window, no console required.

The web UI is built into the app, so there are no extra files to keep with it.
Account data is saved into the folder you choose on the login screen
(blank = your Home folder), as NAME_accountINFOCARD.txt.
EOF

else
  echo "Unknown platform: $PLATFORM" >&2
  exit 1
fi

rm -f "$ARCHIVE"
if [[ "$PLATFORM" == "macos" ]]; then
  # Use ditto for .app bundles; plain zip can produce bundles that Gatekeeper
  # reports as damaged after download/extract.
  ditto -c -k --sequesterRsrc --keepParent "$STAGING" "$ARCHIVE"
else
  (
    cd dist
    zip -r "$(basename "$ARCHIVE")" "$(basename "$STAGING")"
  )
fi

echo "Created $ARCHIVE"
