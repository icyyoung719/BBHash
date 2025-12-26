#!/usr/bin/env python3
# format.py - cross-platform clang-format

import subprocess
import sys
from pathlib import Path

# clang-format 可执行文件
CLANG_FORMAT = "clang-format"

# 文件匹配规则
PATTERNS = [
    "include/*.h",
    "include/*.hpp",
    "tests/*.cpp",
    "examples/*.cpp",
]

def find_files(patterns):
    files = []
    for pat in patterns:
        files.extend(Path().glob(pat))
    return [str(f) for f in files if f.is_file()]

def main():
    files = find_files(PATTERNS)
    if not files:
        print("No files to format.")
        return

    for f in files:
        subprocess.run([CLANG_FORMAT, "-i", f], check=True)
        print(f"Formatted: {f}")

    print("All done.")

if __name__ == "__main__":
    main()
