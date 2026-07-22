#!/usr/bin/env python3
"""독립 Web UI HTML 원본과 펌웨어 임베디드 헤더를 동기화한다."""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import sys


REPO_ROOT = Path(__file__).resolve().parents[1]
HTML_PATH = REPO_ROOT / "web" / "web_ui.html"
HEADER_PATH = REPO_ROOT / "include" / "web" / "web_ui.h"
WEB_UI_PATTERN = re.compile(
    r'(?P<prefix>const char WEB_UI_HTML\[\]\s*=\s*R"rawliteral\()'
    r'(?P<body>[\s\S]*?)'
    r'(?P<suffix>\)rawliteral";)',
)


def read_header() -> tuple[str, re.Match[str]]:
    source = HEADER_PATH.read_text(encoding="utf-8")
    match = WEB_UI_PATTERN.search(source)
    if not match:
        raise RuntimeError(f"WEB_UI_HTML raw literal을 찾지 못했습니다: {HEADER_PATH}")
    return source, match


def read_html() -> str:
    html = HTML_PATH.read_text(encoding="utf-8")
    if html.endswith("\n"):
        html = html[:-1]
    if ")rawliteral\";" in html:
        raise RuntimeError("HTML에 C++ raw literal 종료 문자열이 포함되어 있습니다.")
    return html


def extract() -> None:
    _, match = read_header()
    HTML_PATH.parent.mkdir(parents=True, exist_ok=True)
    HTML_PATH.write_text(match.group("body") + "\n", encoding="utf-8")
    print(f"Web UI 원본 저장: {HTML_PATH.relative_to(REPO_ROOT)}")


def sync() -> None:
    source, match = read_header()
    html = read_html()
    updated = source[: match.start("body")] + html + source[match.end("body") :]
    if updated != source:
        HEADER_PATH.write_text(updated, encoding="utf-8")
        print(f"임베디드 헤더 갱신: {HEADER_PATH.relative_to(REPO_ROOT)}")
    else:
        print("임베디드 헤더가 이미 Web UI 원본과 같습니다.")


def check() -> None:
    _, match = read_header()
    if match.group("body") != read_html():
        print("Web UI 원본과 임베디드 헤더가 다릅니다. --sync를 실행하세요.", file=sys.stderr)
        raise SystemExit(1)
    print("Web UI 동기화 확인 통과")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--extract", action="store_true", help="헤더의 현재 Web UI를 HTML 원본으로 저장")
    action.add_argument("--sync", action="store_true", help="HTML 원본을 임베디드 헤더에 반영")
    action.add_argument("--check", action="store_true", help="HTML 원본과 임베디드 헤더가 같은지 검사")
    args = parser.parse_args()

    if args.extract:
        extract()
    elif args.sync:
        sync()
    else:
        check()


if __name__ == "__main__":
    main()
