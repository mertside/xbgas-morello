#!/bin/sh
#
# System Information and Test Environment Validator
# Displays system configuration and validates test environment
#
# Usage: ./system_info.sh
#

set -e

echo "============================================================"
echo "🖥️  xBGAS-Morello TTU Test Environment Information" 
echo "============================================================"

echo ""
echo "📅 Date and Time:"
echo "-------------------"
date

echo ""
echo "🔧 System Information:"
echo "----------------------"
uname -a

echo ""
echo "💾 Hardware Configuration:"
echo "--------------------------"
if command -v sysctl >/dev/null 2>&1; then
    sysctl -a 2>/dev/null | egrep -i 'hw.machine|hw.model|hw.ncpu|hw.usermem' || echo "Hardware info not available via sysctl"
elif [ -f "/proc/cpuinfo" ]; then
    echo "CPU Info:"
    grep -E "model name|processor|cpu cores|MemTotal" /proc/cpuinfo /proc/meminfo 2>/dev/null | head -10
else
    echo "Hardware information not available"
fi

echo ""
echo "🛠️  Compiler Information:"
echo "-------------------------"
if command -v cc >/dev/null 2>&1; then
    echo "Default C compiler (cc):"
    cc --version 2>/dev/null || echo "cc version info not available"
else
    echo "No 'cc' compiler found"
fi

if command -v gcc >/dev/null 2>&1; then
    echo ""
    echo "GCC compiler:"
    gcc --version 2>/dev/null | head -1 || echo "gcc version info not available"
fi

if command -v clang >/dev/null 2>&1; then
    echo ""
    echo "Clang compiler:"
    clang --version 2>/dev/null | head -1 || echo "clang version info not available"
fi

echo ""
echo "🔒 CHERI/Morello Environment:"
echo "-----------------------------"
# Check for CHERI-specific environment indicators
if uname -a | grep -i morello >/dev/null 2>&1; then
    echo "✓ Running on Morello system"
elif uname -a | grep -i cheri >/dev/null 2>&1; then
    echo "✓ Running on CHERI system"
else
    echo "⚠ System type: $(uname -m) - may not be CHERI/Morello"
fi

# Check for capability support
if [ -f "/proc/sys/abi/cheri_enabled" ]; then
    cheri_status=$(cat /proc/sys/abi/cheri_enabled 2>/dev/null || echo "unknown")
    echo "CHERI capabilities: $cheri_status"
fi

echo ""
echo "📁 Project Environment:"
echo "-----------------------"
echo "Current directory: $(pwd)"
echo "User: $(whoami)"

if [ -f "Makefile_objaware" ]; then
    echo "✓ Makefile_objaware found"
else
    echo "⚠ Makefile_objaware not found"
fi

if [ -d "../../runtime" ]; then
    echo "✓ Runtime directory exists"
    if [ -f "../../runtime/xbrtime_morello.h" ]; then
        echo "✓ xbrtime_morello.h found"
    else
        echo "⚠ xbrtime_morello.h not found"
    fi
else
    echo "⚠ Runtime directory not found"
fi

# Count test files
refactored_count=$(ls -1 *_refactored*.c 2>/dev/null | wc -l | tr -d ' ')
echo "Refactored test files: $refactored_count"

echo ""
echo "🔧 Build Tools:"
echo "---------------"
if command -v make >/dev/null 2>&1; then
    echo "✓ make: $(make --version 2>/dev/null | head -1 || echo 'available')"
else
    echo "⚠ make not found"
fi

if command -v bmake >/dev/null 2>&1; then
    echo "✓ bmake: $(bmake --version 2>/dev/null | head -1 || echo 'available')"
else
    echo "- bmake not available"
fi

echo ""
echo "============================================================"
echo "Environment validation complete."
echo "============================================================"
