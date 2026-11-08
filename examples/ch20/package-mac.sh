#!/usr/bin/env bash
# Example for chapter 20 of "Learn Unreal Engine 5".
# Packages a project for macOS with RunUAT BuildCookRun.
#
# Usage:   ./package-mac.sh [Shipping|Development]
# Edit ENGINE_DIR, PROJECT and OUTPUT_DIR below first (or set them in the environment).
# The flags are explained in package-windows.bat; only -platform differs.
# Xcode must be installed. Giving the .app to other people also needs code signing
# and notarization, which this script does not do.

set -euo pipefail

ENGINE_DIR="${ENGINE_DIR:-/Users/Shared/Epic Games/UE_5.8}"
PROJECT="${PROJECT:-$HOME/Projects/GridSnake/GridSnake.uproject}"
OUTPUT_DIR="${OUTPUT_DIR:-$HOME/Builds/GridSnake}"
CONFIG="${1:-Shipping}"

UAT="$ENGINE_DIR/Engine/Build/BatchFiles/RunUAT.sh"

if [[ ! -x "$UAT" ]]; then
  echo "RunUAT.sh not found at $UAT - set ENGINE_DIR." >&2
  exit 1
fi
if [[ ! -f "$PROJECT" ]]; then
  echo "$PROJECT not found - set PROJECT." >&2
  exit 1
fi
case "$CONFIG" in
  Shipping|Development|DebugGame|Test) ;;
  *) echo "Unknown configuration '$CONFIG'." >&2; exit 1 ;;
esac

"$UAT" BuildCookRun \
  -project="$PROJECT" \
  -noP4 \
  -unattended \
  -utf8output \
  -platform=Mac \
  -clientconfig="$CONFIG" \
  -build \
  -cook \
  -stage \
  -pak \
  -iostore \
  -compressed \
  -package \
  -archive \
  -archivedirectory="$OUTPUT_DIR"

echo "Packaged $CONFIG build in $OUTPUT_DIR/Mac"
