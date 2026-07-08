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
  APP="BankAccount"
elif [[ "$PLATFORM" == "windows" ]]; then
  APP="BankAccount.exe"
else
  echo "Unknown platform: $PLATFORM" >&2
  exit 1
fi

if [[ ! -f "bin/${APP}" ]]; then
  echo "Missing bin/${APP} — run 'make app' first" >&2
  exit 1
fi

cp "bin/${APP}" "$STAGING/"
chmod +x "$STAGING/${APP}" 2>/dev/null || true
cp LICENSE "$STAGING/"

cat > "$STAGING/RUN.txt" <<'EOF'
Bank Account
============

This is a single, self-contained application — the web UI is built in, so
there are no extra files to keep alongside it.

macOS
-----
  Double-click "BankAccount" (or run ./BankAccount in a terminal).
  If macOS blocks it: right-click -> Open, then confirm.

Windows
-------
  Double-click "BankAccount.exe".

Account data is saved as NAME_accountINFOCARD.txt next to the app.
EOF

rm -f "$ARCHIVE"
(
  cd dist
  zip -r "$(basename "$ARCHIVE")" "$(basename "$STAGING")"
)

echo "Created $ARCHIVE"
