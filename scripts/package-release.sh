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
mkdir -p "$STAGING/web"

copy_bin() {
  local name="$1"
  if [[ -f "bin/${name}" ]]; then
    cp "bin/${name}" "$STAGING/"
    chmod +x "$STAGING/${name}"
  else
    echo "Missing bin/${name}" >&2
    exit 1
  fi
}

if [[ "$PLATFORM" == "macos" ]]; then
  copy_bin account
  copy_bin account_gui
  copy_bin account_web
elif [[ "$PLATFORM" == "windows" ]]; then
  for name in account.exe account_gui.exe account_web.exe; do
    if [[ -f "bin/${name}" ]]; then
      cp "bin/${name}" "$STAGING/"
    else
      echo "Missing bin/${name}" >&2
      exit 1
    fi
  done
else
  echo "Unknown platform: $PLATFORM" >&2
  exit 1
fi

cp web/index.html web/styles.css web/app.js "$STAGING/web/"
cp LICENSE "$STAGING/"

cat > "$STAGING/RUN.txt" <<'EOF'
Bank Account Program
====================

Keep this folder together. The programs look for the web/ folder next to them.

macOS
-----
  ./account_gui     Native window (recommended)
  ./account_web     Open http://127.0.0.1:8765 in your browser
  ./account         Terminal menu version

Windows
-------
  account_gui.exe   Native window (FLTK)
  account_web.exe   Open http://127.0.0.1:8765 in your browser
  account.exe       Terminal menu version

Account files are saved as NAME_accountINFOCARD.txt in this folder.
EOF

rm -f "$ARCHIVE"
(
  cd dist
  zip -r "$(basename "$ARCHIVE")" "$(basename "$STAGING")"
)

echo "Created $ARCHIVE"
