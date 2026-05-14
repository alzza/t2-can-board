# C/C++ printf 계열 포맷 문자열과 인자 개수를 검사하는 스크립트
from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Iterable, Optional

CALLS = (
    ("Serial.printf", 0),
    ("snprintf", 2),
    ("sprintf", 1),
    ("printf", 0),
)

SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hpp", ".ino"}
SKIP_DIRS = {".git", ".pio", "node_modules", "build", "dist"}


def is_ident_char(ch: str) -> bool:
    return ch.isalnum() or ch == "_"


def find_matching_paren(text: str, open_pos: int) -> int:
    depth = 0
    i = open_pos
    state = "code"
    while i < len(text):
        ch = text[i]
        nxt = text[i + 1] if i + 1 < len(text) else ""
        if state == "code":
            if ch == '"':
                state = "string"
            elif ch == "'":
                state = "char"
            elif ch == "/" and nxt == "/":
                state = "line_comment"
                i += 1
            elif ch == "/" and nxt == "*":
                state = "block_comment"
                i += 1
            elif ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
                if depth == 0:
                    return i
        elif state == "string":
            if ch == "\\":
                i += 1
            elif ch == '"':
                state = "code"
        elif state == "char":
            if ch == "\\":
                i += 1
            elif ch == "'":
                state = "code"
        elif state == "line_comment":
            if ch == "\n":
                state = "code"
        elif state == "block_comment":
            if ch == "*" and nxt == "/":
                state = "code"
                i += 1
        i += 1
    return -1


def split_top_level_args(arg_text: str) -> list[str]:
    args: list[str] = []
    start = 0
    depth = 0
    i = 0
    state = "code"
    while i < len(arg_text):
        ch = arg_text[i]
        nxt = arg_text[i + 1] if i + 1 < len(arg_text) else ""
        if state == "code":
            if ch == '"':
                state = "string"
            elif ch == "'":
                state = "char"
            elif ch == "/" and nxt == "/":
                state = "line_comment"
                i += 1
            elif ch == "/" and nxt == "*":
                state = "block_comment"
                i += 1
            elif ch in "([{":
                depth += 1
            elif ch in ")]}" and depth > 0:
                depth -= 1
            elif ch == "," and depth == 0:
                args.append(arg_text[start:i].strip())
                start = i + 1
        elif state == "string":
            if ch == "\\":
                i += 1
            elif ch == '"':
                state = "code"
        elif state == "char":
            if ch == "\\":
                i += 1
            elif ch == "'":
                state = "code"
        elif state == "line_comment":
            if ch == "\n":
                state = "code"
        elif state == "block_comment":
            if ch == "*" and nxt == "/":
                state = "code"
                i += 1
        i += 1
    tail = arg_text[start:].strip()
    if tail:
        args.append(tail)
    return args


def parse_one_string_literal(text: str, pos: int) -> tuple[Optional[str], int]:
    prefixes = ("u8", "L", "u", "U")
    for prefix in prefixes:
        if text.startswith(prefix, pos) and pos + len(prefix) < len(text) and text[pos + len(prefix)] == '"':
            pos += len(prefix)
            break
    if pos >= len(text) or text[pos] != '"':
        return None, pos

    pos += 1
    chars: list[str] = []
    while pos < len(text):
        ch = text[pos]
        if ch == "\\":
            if pos + 1 < len(text):
                chars.append(text[pos + 1])
                pos += 2
                continue
        if ch == '"':
            return "".join(chars), pos + 1
        chars.append(ch)
        pos += 1
    return None, pos


def extract_literal_format(arg: str) -> Optional[str]:
    pos = 0
    chunks: list[str] = []
    while True:
        while pos < len(arg) and arg[pos].isspace():
            pos += 1
        if pos >= len(arg):
            break
        chunk, pos2 = parse_one_string_literal(arg, pos)
        if chunk is None:
            return None
        chunks.append(chunk)
        pos = pos2
    return "".join(chunks) if chunks else None


def count_printf_args(fmt: str) -> int:
    count = 0
    i = 0
    while i < len(fmt):
        if fmt[i] != "%":
            i += 1
            continue
        i += 1
        if i < len(fmt) and fmt[i] == "%":
            i += 1
            continue

        while i < len(fmt) and fmt[i] in "-+ #0'":
            i += 1

        if i < len(fmt) and fmt[i] == "*":
            count += 1
            i += 1
        else:
            while i < len(fmt) and fmt[i].isdigit():
                i += 1

        if i < len(fmt) and fmt[i] == ".":
            i += 1
            if i < len(fmt) and fmt[i] == "*":
                count += 1
                i += 1
            else:
                while i < len(fmt) and fmt[i].isdigit():
                    i += 1

        if fmt.startswith(("hh", "ll"), i):
            i += 2
        elif i < len(fmt) and fmt[i] in "hljztL":
            i += 1

        if i < len(fmt):
            if fmt[i] != "%":
                count += 1
            i += 1
    return count


def match_call(text: str, pos: int) -> Optional[tuple[str, int, int]]:
    for name, fmt_index in CALLS:
        if not text.startswith(name, pos):
            continue
        before = text[pos - 1] if pos > 0 else ""
        after_pos = pos + len(name)
        after = text[after_pos] if after_pos < len(text) else ""
        if after != "(":
            continue
        if name == "printf" and before in ".:" or (before and is_ident_char(before)):
            continue
        if name != "Serial.printf" and before and is_ident_char(before):
            continue
        return name, fmt_index, after_pos
    return None


def iter_call_results(path: Path, text: str) -> Iterable[str]:
    i = 0
    state = "code"
    while i < len(text):
        ch = text[i]
        nxt = text[i + 1] if i + 1 < len(text) else ""
        if state == "code":
            if ch == '"':
                state = "string"
            elif ch == "'":
                state = "char"
            elif ch == "/" and nxt == "/":
                state = "line_comment"
                i += 1
            elif ch == "/" and nxt == "*":
                state = "block_comment"
                i += 1
            else:
                match = match_call(text, i)
                if match:
                    name, fmt_index, open_pos = match
                    close_pos = find_matching_paren(text, open_pos)
                    if close_pos == -1:
                        i += len(name)
                        continue
                    args = split_top_level_args(text[open_pos + 1:close_pos])
                    if len(args) <= fmt_index:
                        i = close_pos + 1
                        continue
                    fmt = extract_literal_format(args[fmt_index])
                    if fmt is None:
                        i = close_pos + 1
                        continue
                    expected = count_printf_args(fmt)
                    actual = len(args) - fmt_index - 1
                    if expected != actual:
                        line = text.count("\n", 0, i) + 1
                        yield f"{path}:{line}: {name} expects {expected} format args, got {actual}"
                    i = close_pos
        elif state == "string":
            if ch == "\\":
                i += 1
            elif ch == '"':
                state = "code"
        elif state == "char":
            if ch == "\\":
                i += 1
            elif ch == "'":
                state = "code"
        elif state == "line_comment":
            if ch == "\n":
                state = "code"
        elif state == "block_comment":
            if ch == "*" and nxt == "/":
                state = "code"
                i += 1
        i += 1


def source_files(paths: list[Path]) -> list[Path]:
    files: list[Path] = []
    for path in paths:
        if path.is_file() and path.suffix in SOURCE_SUFFIXES:
            files.append(path)
        elif path.is_dir():
            for child in path.rglob("*"):
                if any(part in SKIP_DIRS for part in child.parts):
                    continue
                if child.is_file() and child.suffix in SOURCE_SUFFIXES:
                    files.append(child)
    return sorted(set(files))


def run_self_test() -> int:
    bad = 'void f(){ char b[32]; snprintf(b, sizeof(b), "%u,%s", (unsigned)x, name, extra); }'
    good = 'void f(){ char b[32]; snprintf(b, sizeof(b), "%u,%s", (unsigned)x, name); Serial.printf("%lu %s\\n", now, label); }'
    bad_hits = list(iter_call_results(Path("bad.cpp"), bad))
    good_hits = list(iter_call_results(Path("good.cpp"), good))
    if len(bad_hits) != 1 or good_hits:
        print("self-test failed", file=sys.stderr)
        for hit in bad_hits + good_hits:
            print(hit, file=sys.stderr)
        return 1
    print("self-test passed")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Check literal printf/snprintf argument counts in C/C++ sources.")
    parser.add_argument("paths", nargs="*", help="Files or directories to scan. Defaults to include src test and root C/C++ files.")
    parser.add_argument("--self-test", action="store_true", help="Run built-in parser sanity checks.")
    args = parser.parse_args()

    if args.self_test:
        return run_self_test()

    if args.paths:
        roots = [Path(p) for p in args.paths]
    else:
        roots = [Path("include"), Path("src"), Path("test")] + [p for p in Path(".").iterdir() if p.is_file() and p.suffix in SOURCE_SUFFIXES]

    files = source_files(roots)
    errors: list[str] = []
    for path in files:
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            text = path.read_text(encoding="utf-8", errors="ignore")
        errors.extend(iter_call_results(path, text))

    for error in errors:
        print(error)
    if errors:
        print(f"printf format check failed: {len(errors)} mismatch(es)", file=sys.stderr)
        return 1
    print(f"printf format check passed: {len(files)} file(s) scanned")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
