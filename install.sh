#!/usr/bin/env bash
# Vit & Vito Ecosystem One-Line Bash Installer for Linux & macOS
# Usage: curl -fsSL https://raw.githubusercontent.com/longgoll/vit/main/install.sh | bash

set -e

echo "============================================================"
echo "     ⚡ Vit & Vito Language & Framework Installer (v2.0)    "
echo "============================================================"
echo ""

INSTALL_DIR="$(pwd)"
BIN_DIR="$INSTALL_DIR/bin"
VIT_EXE="$BIN_DIR/vit"

if [ ! -f "$VIT_EXE" ]; then
    echo "[Installer] Error: vit binary not found in $BIN_DIR"
    echo "[Installer] Please build or unpack the Vit release bundle."
    exit 1
fi

chmod +x "$VIT_EXE"
chmod +x "$BIN_DIR/vit-lsp" 2>/dev/null || true

echo "[1/2] Configuring Environment Variables (VIT_HOME & PATH)..."
SHELL_CONFIG=""
if [ -n "$BASH_VERSION" ]; then
    SHELL_CONFIG="$HOME/.bashrc"
elif [ -n "$ZSH_VERSION" ]; then
    SHELL_CONFIG="$HOME/.zshrc"
else
    SHELL_CONFIG="$HOME/.profile"
fi

if ! grep -q "VIT_HOME" "$SHELL_CONFIG" 2>/dev/null; then
    echo "" >> "$SHELL_CONFIG"
    echo "# Vit & Vito Environment Configuration" >> "$SHELL_CONFIG"
    echo "export VIT_HOME=\"$INSTALL_DIR\"" >> "$SHELL_CONFIG"
    echo "export PATH=\"\$VIT_HOME/bin:\$PATH\"" >> "$SHELL_CONFIG"
    echo "      Added VIT_HOME and PATH entry to $SHELL_CONFIG"
else
    echo "      VIT_HOME already exists in $SHELL_CONFIG"
fi

echo "[2/2] Running Vit Toolchain Self-Diagnostics..."
"$VIT_EXE" version

echo ""
echo "============================================================"
echo " 🎉 Vit & Vito installed successfully!"
echo "============================================================"
echo " Quick Start Commands:"
echo "   vit version              Check installed version"
echo "   vit run main.vit         Execute a Vit file instantly"
echo "   vit init my-app          Create a new Vit/Vito project"
echo ""
echo " Note: Please run 'source $SHELL_CONFIG' or restart your shell."
