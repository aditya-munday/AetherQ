#!/usr/bin/env python3
import os
import re
import subprocess
import sys

SOURCE_SUFFIXES = (".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".ipp", ".inl",
                   ".cu", ".cuh", ".py", ".sh", ".bash", ".yml", ".yaml", ".cmake", ".json", ".txt")
CPP_SUFFIXES = (".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".ipp", ".inl",
                ".cu", ".cuh")
SPECIAL_FILES = ("CMakeLists.txt", "CMakePresets.json", "vcpkg.json")
ALLOWED_MARKDOWN = {"README.md"}
ALLOWED_LICENSE = {"LICENSE", "LICENSE.md", "LICENSE.txt"}

BLOCK_COMMENT = re.compile(r"/\*")
LINE_SLASH_COMMENT = re.compile(r"//")
SHEBANG = re.compile(r"^#!")
HASH_COMMENT = re.compile(r"#")
DOCSTRING = re.compile(r"^\s*(?:[rubf]{0,2})('''|\"\"\")")


def tracked_files():
    output = subprocess.check_output(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard"], text=True
    )
    return sorted({line.strip() for line in output.splitlines() if line.strip()})


def is_source(path):
    if path.endswith(SOURCE_SUFFIXES) or os.path.basename(path) in SPECIAL_FILES:
        return True
    return False


def strip_strings(line):
    result = []
    quote = None
    escaped = False
    for character in line:
        if escaped:
            result.append(" ")
            escaped = False
            continue
        if quote is not None:
            if character == "\\":
                escaped = True
                result.append(" ")
                continue
            if character == quote:
                quote = None
            result.append(" ")
            continue
        if character in ("'", '"'):
            quote = character
            result.append(" ")
            continue
        result.append(character)
    return "".join(result)


def check_cpp_like(path, lines):
    findings = []
    for number, raw in enumerate(lines, start=1):
        line = raw.rstrip("\n")
        if SHEBANG.match(line):
            continue
        scrubbed = strip_strings(line)
        if BLOCK_COMMENT.search(scrubbed):
            findings.append((path, number, "block comment"))
        if LINE_SLASH_COMMENT.search(scrubbed):
            findings.append((path, number, "line comment"))
    return findings


def strip_python_strings(line):
    result = []
    quote = None
    escaped = False
    for character in line:
        if escaped:
            result.append(" ")
            escaped = False
            continue
        if quote is not None:
            if character == "\\":
                escaped = True
                result.append(" ")
                continue
            if character == quote:
                quote = None
            result.append(" ")
            continue
        if character in ("'", '"'):
            quote = character
            result.append(" ")
            continue
        result.append(character)
    return "".join(result)


def check_hash_language(path, lines):
    findings = []
    in_docstring = None
    for number, raw in enumerate(lines, start=1):
        line = raw.rstrip("\n")
        if SHEBANG.match(line):
            continue
        match = DOCSTRING.match(line)
        if match is not None:
            marker = match.group(1)
            if in_docstring is None:
                in_docstring = marker
                if line.count(marker) >= 2:
                    in_docstring = None
            elif in_docstring == marker:
                in_docstring = None
            findings.append((path, number, "docstring"))
            continue
        if in_docstring is not None:
            findings.append((path, number, "docstring"))
            continue
        scrubbed = strip_python_strings(line)
        if HASH_COMMENT.search(scrubbed):
            findings.append((path, number, "comment"))
    return findings


def check_markdown(files):
    findings = []
    for path in files:
        if path.lower().endswith(".md"):
            base = os.path.basename(path)
            if base not in ALLOWED_MARKDOWN and base not in ALLOWED_LICENSE:
                findings.append((path, 0, "unexpected markdown file"))
    return findings


def main():
    files = tracked_files()
    findings = []
    for path in files:
        if not is_source(path):
            continue
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            lines = handle.readlines()
        if path.endswith(CPP_SUFFIXES):
            findings.extend(check_cpp_like(path, lines))
        else:
            findings.extend(check_hash_language(path, lines))
    findings.extend(check_markdown(files))

    if findings:
        for path, number, kind in findings:
            location = f"{path}:{number}" if number else path
            print(f"{location}: {kind}")
        print(f"policy violations: {len(findings)}")
        return 1

    print(f"policy check passed over {len(files)} tracked files")
    return 0


if __name__ == "__main__":
    sys.exit(main())
