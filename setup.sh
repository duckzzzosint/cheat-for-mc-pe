#!/bin/sh
# Run this ONCE after cloning — downloads gradle-wrapper.jar for local builds.
# GitHub Actions doesn't need this (uses setup-gradle action directly).

set -e

JAR="gradle/wrapper/gradle-wrapper.jar"
URL="https://github.com/gradle/gradle/raw/v8.5.0/gradle/wrapper/gradle-wrapper.jar"

if [ -f "$JAR" ]; then
    echo "[ok] $JAR already exists"
    exit 0
fi

echo "[*] Downloading gradle-wrapper.jar..."
if command -v curl >/dev/null 2>&1; then
    curl -fsSL "$URL" -o "$JAR"
elif command -v wget >/dev/null 2>&1; then
    wget -q "$URL" -O "$JAR"
else
    echo "[!] Neither curl nor wget found. Download manually:"
    echo "    $URL"
    echo "    -> $JAR"
    exit 1
fi

chmod +x gradlew
echo "[ok] Done. Run: ./gradlew assembleDebug"
