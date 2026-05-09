#!/bin/bash
umask 007

#R001: Run with bash in strict fail-fast mode.
set -euo pipefail

print_header() {
    echo "============================================================"
    echo "Fountain Prerequisites Installer"
    echo "============================================================"
    echo ""
}

ensure_homebrew() {
    #R005: Verify Homebrew exists before package actions.
    echo "[Homebrew] Checking..."
    if command -v brew >/dev/null 2>&1; then
        echo "✅ [Homebrew] Installed"
    else
        echo "❌ [Homebrew] Not installed."
        echo "Install Homebrew and rerun:"
        echo "/bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
        exit 1
    fi
}

ensure_xcode_toolchain() {
    #R010: Ensure xcodebuild, xcrun, and clang++ are available.
    local missing=0
    echo "[Xcode Toolchain] Checking..."
    if command -v xcodebuild >/dev/null 2>&1; then
        echo "✅ [Xcode Toolchain] xcodebuild available"
    else
        echo "❌ [Xcode Toolchain] xcodebuild not found"
        missing=1
    fi
    if command -v xcrun >/dev/null 2>&1; then
        echo "✅ [Xcode Toolchain] xcrun available"
    else
        echo "❌ [Xcode Toolchain] xcrun not found"
        missing=1
    fi
    if command -v clang++ >/dev/null 2>&1; then
        echo "✅ [Xcode Toolchain] clang++ available"
    else
        echo "❌ [Xcode Toolchain] clang++ not found"
        missing=1
    fi
    if [ "$missing" -ne 0 ]; then
        echo "Install Xcode or Command Line Tools and rerun (xcode-select --install)."
        exit 1
    fi
}

ensure_brew_formula() {
    #R015: Install required Homebrew formulas when missing.
    local formula="$1"
    local command_name="${2:-$formula}"
    echo "[${formula}] Checking..."
    if command -v "$command_name" >/dev/null 2>&1; then
        echo "✅ [${formula}] Available on PATH"
        return
    fi
    echo "⚠️  [${formula}] Missing; installing with Homebrew..."
    brew install "$formula"
    if command -v "$command_name" >/dev/null 2>&1; then
        echo "✅ [${formula}] Installed and available"
    else
        echo "❌ [${formula}] Install completed but command is still missing"
        exit 1
    fi
}

ensure_cmake_and_ctest() {
    #R020: Ensure cmake and ctest are installed and available.
    ensure_brew_formula "cmake" "cmake"
    echo "[ctest] Checking..."
    if command -v ctest >/dev/null 2>&1; then
        echo "✅ [ctest] Available on PATH"
    else
        echo "❌ [ctest] Missing after cmake installation"
        exit 1
    fi
}

ensure_core_tools() {
    #R025: Ensure tools used by this C++ repo are available.
    ensure_brew_formula "sqlite3" "sqlite3"
    ensure_brew_formula "pkg-config" "pkg-config"
}

ensure_sast_tools() {
    #R030: Ensure SAST tools for this repository are available.
    ensure_brew_formula "shellcheck" "shellcheck"
    ensure_brew_formula "semgrep" "semgrep"
    ensure_clang_tidy
    ensure_brew_formula "gitleaks" "gitleaks"
}

ensure_clang_tidy() {
    #R030: Ensure clang-tidy exists for Makefile SAST lanes.
    local brew_prefix llvm_prefix llvm_tidy
    echo "[clang-tidy] Checking..."
    if command -v clang-tidy >/dev/null 2>&1; then
        echo "✅ [clang-tidy] Available on PATH"
        return
    fi
    echo "⚠️  [clang-tidy] Missing; installing with Homebrew..."
    if brew install clang-tools-extra; then
        :
    else
        echo "⚠️  [clang-tidy] clang-tools-extra unavailable; falling back to llvm formula..."
        brew install llvm
    fi
    if command -v clang-tidy >/dev/null 2>&1; then
        echo "✅ [clang-tidy] Installed and available"
        return
    fi
    brew_prefix="$(brew --prefix)"
    llvm_prefix="$(brew --prefix llvm)"
    llvm_tidy="${llvm_prefix}/bin/clang-tidy"
    if [ -x "$llvm_tidy" ]; then
        ln -sf "$llvm_tidy" "${brew_prefix}/bin/clang-tidy"
    fi
    if command -v clang-tidy >/dev/null 2>&1; then
        echo "✅ [clang-tidy] Linked into ${brew_prefix}/bin/clang-tidy"
    else
        echo "❌ [clang-tidy] Install completed but command is still missing"
        exit 1
    fi
}

print_final_guidance() {
    #R035: Print Fountain-specific readiness guidance.
    echo ""
    echo "✅ All prerequisites are satisfied for this repository."
    echo ""
    echo "Next commands:"
    echo "- cmake -S . -B build"
    echo "- cmake --build build"
    echo "- ctest --test-dir build --output-on-failure"
}

main() {
    #R040: Keep prerequisite installer idempotent and rerunnable.
    print_header
    ensure_homebrew
    echo ""
    ensure_xcode_toolchain
    ensure_cmake_and_ctest
    ensure_core_tools
    ensure_sast_tools
    print_final_guidance
}

main "$@"
