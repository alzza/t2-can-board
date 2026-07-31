#!/usr/bin/env python3
"""시계열 CSV 헤더 열 수와 snprintf 데이터 값 수를 정적으로 대조한다."""

from __future__ import annotations

import re
import sys
from pathlib import Path

from check_printf_formats import count_printf_args


ROOT = Path(__file__).resolve().parents[1]
TIMESERIES = ROOT / "include" / "timeseries.h"
WEB_SERVER = ROOT / "include" / "web" / "web_server.h"
STRING_RE = re.compile(r'"((?:\\.|[^"\\])*)"')


def fail(message: str) -> None:
    print(f"시계열 CSV 스키마 검사 실패: {message}", file=sys.stderr)
    raise SystemExit(1)


def between(text: str, start: str, end: str, offset: int = 0) -> tuple[str, int]:
    begin = text.find(start, offset)
    if begin < 0:
        fail(f"시작 표식을 찾지 못함: {start}")
    finish = text.find(end, begin)
    if finish < 0:
        fail(f"종료 표식을 찾지 못함: {end}")
    return text[begin:finish], finish


def joined_literals(block: str) -> str:
    return "".join(match.group(1) for match in STRING_RE.finditer(block))


def column_count(header: str) -> int:
    return len(header.split(","))


def direct_csv_counts(text: str) -> tuple[int, int]:
    header_block, header_end = between(
        text, "const char* hdr =", "httpd_resp_sendstr_chunk(req, hdr)"
    )
    first_format_block, first_end = between(
        text, "int used = snprintf", "wallTime,", header_end
    )
    second_format_block, _ = between(
        text, "snprintf(line + used", "(unsigned)s.aFrames", first_end
    )
    header = joined_literals(header_block)
    formats = joined_literals(first_format_block) + joined_literals(second_format_block)
    # schema_version은 포맷 인자가 아니라 문자열의 고정값 "3"으로 출력한다.
    return column_count(header), 1 + count_printf_args(formats)


def bundle_csv_counts(text: str) -> tuple[int, int]:
    header_anchor = text.find("wall_time,timestamp_ms,busoff")
    if header_anchor < 0:
        fail("통합 로그 시계열 헤더를 찾지 못함")
    header_start = text.rfind("httpd_resp_sendstr_chunk(req,", 0, header_anchor)
    if header_start < 0:
        fail("통합 로그 시계열 헤더 호출 시작을 찾지 못함")
    header_end = text.find(");", header_anchor)
    if header_end < 0:
        fail("통합 로그 시계열 헤더 호출 끝을 찾지 못함")
    header_block = text[header_start:header_end]
    first_format_block, first_end = between(
        text, "int used = snprintf", "tsBuf, (unsigned)s.t_ms", header_end
    )
    second_format_block, _ = between(
        text, "snprintf(line + used", "(unsigned)s.aFrames", first_end
    )
    header = joined_literals(header_block)
    formats = joined_literals(first_format_block) + joined_literals(second_format_block)
    return column_count(header), count_printf_args(formats)


def main() -> None:
    direct_header, direct_values = direct_csv_counts(
        TIMESERIES.read_text(encoding="utf-8")
    )
    bundle_header, bundle_values = bundle_csv_counts(
        WEB_SERVER.read_text(encoding="utf-8")
    )

    if direct_header != direct_values:
        fail(
            f"/api/timeseries.csv 헤더={direct_header}, 데이터={direct_values}"
        )
    if bundle_header != bundle_values:
        fail(f"통합 로그 시계열 헤더={bundle_header}, 데이터={bundle_values}")

    print(
        "시계열 CSV 스키마 검사 통과: "
        f"독립 CSV {direct_header}열, 통합 로그 {bundle_header}열"
    )


if __name__ == "__main__":
    main()
