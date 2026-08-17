#!/usr/bin/env python3
"""시계열·이벤트·자가진단 CSV 헤더와 데이터 열 수를 정적으로 대조한다."""

from __future__ import annotations

import re
import sys
from pathlib import Path

from check_printf_formats import count_printf_args


ROOT = Path(__file__).resolve().parents[1]
TIMESERIES = ROOT / "include" / "timeseries.h"
WEB_SERVER = ROOT / "include" / "web" / "web_server.h"
EVENT_LOG = ROOT / "include" / "event_log.h"
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
    # schema_version은 포맷 인자가 아니라 문자열의 고정값으로 출력한다.
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


def single_row_csv_counts(text: str, function_name: str) -> tuple[int, list[int]]:
    function_start = text.find(function_name)
    if function_start < 0:
        fail(f"CSV 함수를 찾지 못함: {function_name}")
    function_end = text.find("\n}\n", function_start)
    if function_end < 0:
        fail(f"CSV 함수 끝을 찾지 못함: {function_name}")
    block = text[function_start:function_end]
    header_anchor = block.find("schema_version,firmware_version")
    if header_anchor < 0:
        fail(f"CSV 헤더를 찾지 못함: {function_name}")
    header_call_start = block.rfind("httpd_resp_sendstr_chunk(req", 0, header_anchor)
    header_call_end = block.find(");", header_anchor)
    header = joined_literals(block[header_call_start:header_call_end])

    value_counts: list[int] = []
    offset = 0
    while True:
        row_start = block.find("snprintf(line, sizeof(line)", offset)
        if row_start < 0:
            break
        args_start = block.find("FIRMWARE_VERSION", row_start)
        if args_start < 0:
            fail(f"CSV 행 인자 시작을 찾지 못함: {function_name}")
        formats = joined_literals(block[row_start:args_start])
        value_counts.append(column_count(formats))
        offset = args_start + len("FIRMWARE_VERSION")
    if not value_counts:
        fail(f"CSV 데이터 행을 찾지 못함: {function_name}")
    return column_count(header), value_counts


def event_csv_counts(text: str) -> tuple[int, int]:
    header_block, _ = between(text, "inline void eventLogCsvHeader", "inline esp_err_t eventLogCsvRow")
    row_block, _ = between(text, "inline esp_err_t eventLogCsvRow", "inline esp_err_t eventLogCsvHandler")
    header = joined_literals(header_block)
    format_end = row_block.find("FIRMWARE_VERSION")
    if format_end < 0:
        fail("이벤트 CSV 데이터 인자 시작을 찾지 못함")
    formats = joined_literals(row_block[:format_end])
    return column_count(header), column_count(formats)


def main() -> None:
    direct_header, direct_values = direct_csv_counts(
        TIMESERIES.read_text(encoding="utf-8")
    )
    bundle_header, bundle_values = bundle_csv_counts(
        WEB_SERVER.read_text(encoding="utf-8")
    )
    event_header, event_values = event_csv_counts(
        EVENT_LOG.read_text(encoding="utf-8")
    )
    diag_header, diag_value_rows = single_row_csv_counts(
        WEB_SERVER.read_text(encoding="utf-8"), "static esp_err_t canDiagLogDlHandler"
    )

    if direct_header != direct_values:
        fail(
            f"/api/timeseries.csv 헤더={direct_header}, 데이터={direct_values}"
        )
    if bundle_header != bundle_values:
        fail(f"통합 로그 시계열 헤더={bundle_header}, 데이터={bundle_values}")
    if event_header != event_values:
        fail(f"이벤트 CSV 헤더={event_header}, 데이터={event_values}")
    for row, diag_values in enumerate(diag_value_rows, start=1):
        if diag_header != diag_values:
            fail(f"자가진단 CSV {row}번 행 헤더={diag_header}, 데이터={diag_values}")

    print(
        "시계열 CSV 스키마 검사 통과: "
        f"시계열 {direct_header}열, 통합 시계열 {bundle_header}열, "
        f"이벤트 {event_header}열, 자가진단 {diag_header}열"
    )


if __name__ == "__main__":
    main()
