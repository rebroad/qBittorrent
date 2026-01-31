#!/usr/bin/env bash
# Build qBittorrent on Ubuntu with Qt 6.6 and system deps.
# Run on a clean Ubuntu (22.04 / 24.04) to install everything and build.
# Usage: ./build-qbittorrent-ubuntu.sh [path-to-qBittorrent-source]
#        If path is omitted, uses ~/src/qBittorrent.

set -e

QBITTORRENT_SRC="${1:-$HOME/src/qBittorrent}"
QT_VERSION="${QT_VERSION:-6.6.3}"
QT_DIR="${QT_DIR:-$HOME/Qt}"
QT_PREFIX="$QT_DIR/$QT_VERSION/gcc_64"
BUILD_DIR_NAME="${BUILD_DIR_NAME:-build-qt663}"

echo "=== qBittorrent build script ==="
echo "Source: $QBITTORRENT_SRC"
echo "Qt $QT_VERSION prefix: $QT_PREFIX"
echo ""

# --- 1) Install apt packages ---
echo ">>> Installing apt dependencies..."
sudo apt-get update -qq
sudo apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  libssl-dev \
  zlib1g-dev \
  libtorrent-rasterbar-dev \
  libboost-dev \
  libxkbcommon-x11-dev \
  libxcb-cursor-dev \
  pkg-config

# Optional: if your Ubuntu has Qt 6.4 and libtorrent is too old, you may need to build
# libtorrent/boost from source (see qBittorrent .github/workflows/ci_ubuntu.yaml).
# This script assumes distro packages are sufficient for 22.04/24.04.

# --- 2) Install Qt 6.6.x via aqt if not present ---
if [ ! -d "$QT_PREFIX" ] || [ ! -f "$QT_PREFIX/bin/qmake" ]; then
  echo ">>> Qt $QT_VERSION not found at $QT_PREFIX. Installing via aqtinstall..."
  if ! command -v aqt >/dev/null 2>&1; then
    echo "    Installing aqtinstall (pip)..."
    pip3 install --user aqtinstall || pip install --user aqtinstall
    AQT="$HOME/.local/bin/aqt"
    [ -x "$AQT" ] || AQT="aqt"
  else
    AQT="aqt"
  fi
  mkdir -p "$QT_DIR"
  # Install Qt desktop (full install; takes a few minutes)
  $AQT install-qt linux desktop "$QT_VERSION" gcc_64 -O "$QT_DIR"
  if [ ! -f "$QT_PREFIX/bin/qmake" ]; then
    echo "ERROR: Qt install failed (no $QT_PREFIX/bin/qmake). Check aqt output above." >&2
    exit 1
  fi
  echo "    Qt installed to $QT_PREFIX"
else
  echo ">>> Using existing Qt at $QT_PREFIX"
fi

# --- 3) Configure and build qBittorrent ---
if [ ! -d "$QBITTORRENT_SRC" ]; then
  echo "ERROR: qBittorrent source directory not found: $QBITTORRENT_SRC" >&2
  echo "Clone it first, e.g.: git clone https://github.com/qbittorrent/qBittorrent.git $QBITTORRENT_SRC" >&2
  exit 1
fi

BUILD_DIR="$QBITTORRENT_SRC/$BUILD_DIR_NAME"
echo ">>> Configuring qBittorrent in $BUILD_DIR..."
mkdir -p "$BUILD_DIR"
cmake -S "$QBITTORRENT_SRC" -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$QT_PREFIX"

echo ">>> Building qBittorrent..."
cmake --build "$BUILD_DIR" -j"$(nproc)"

BINARY="$BUILD_DIR/src/qbittorrent"
if [ -x "$BINARY" ]; then
  echo ""
  echo "=== Build finished ==="
  echo "Binary: $BINARY"
  echo "Run: $BINARY"
else
  echo "WARNING: Expected binary not found at $BINARY" >&2
  exit 1
fi
