#!/usr/bin/env python3
"""T2-CAN 이벤트·시계열 CSV의 구조와 핵심 CAN 안정성 지표를 요약한다."""

from __future__ import annotations

import argparse
import csv
import json
from collections import Counter
from pathlib import Path


def read_csv(path: Path):
    with path.open(encoding="utf-8-sig", newline="") as handle:
        raw = list(csv.reader(handle))
    if not raw:
        raise ValueError(f"empty CSV: {path}")
    header = raw[0]
    bad_rows = [index + 2 for index, row in enumerate(raw[1:]) if len(row) != len(header)]
    with path.open(encoding="utf-8-sig", newline="") as handle:
        rows = list(csv.DictReader(handle))
    return header, rows, bad_rows


def number(row, key):
    return float(row[key])


def summarize_timeseries(path: Path):
    header, rows, bad_rows = read_csv(path)
    if not rows:
        raise ValueError(f"no data rows: {path}")
    return {
        "file": path.name,
        "rows": len(rows),
        "columns": len(header),
        "bad_rows": bad_rows,
        "duplicate_rows": len(rows) - len({tuple(row.items()) for row in rows}),
        "start": rows[0]["wall_time"],
        "end": rows[-1]["wall_time"],
        "duration_ms": int(rows[-1]["uptime_ms"]) - int(rows[0]["uptime_ms"]),
        "b_busoff_min": min(number(row, "b_busoff") for row in rows),
        "b_busoff_max": max(number(row, "b_busoff") for row in rows),
        "b_bus_error_first": number(rows[0], "b_bus_error"),
        "b_bus_error_last": number(rows[-1], "b_bus_error"),
        "b_tec_max": max(number(row, "b_tec") for row in rows),
        "b_rec_max": max(number(row, "b_rec") for row in rows),
        "b_tx_fail_delta": number(rows[-1], "b_tx_fail") - number(rows[0], "b_tx_fail"),
        "b_running_rows": sum(row["b_driver_state"] == "1" for row in rows),
        "a_rx_overrun_delta": number(rows[-1], "a_rx_overrun") - number(rows[0], "a_rx_overrun"),
        "a_queue_drop_delta": number(rows[-1], "a_rx_queue_drops") - number(rows[0], "a_rx_queue_drops"),
        "a_queue_high_water_max": max(number(row, "a_rx_queue_high_water") for row in rows),
        "a_tx_hard_delta": number(rows[-1], "a_tx_hard_error") - number(rows[0], "a_tx_hard_error"),
        "a_guard_samples": sum(number(row, "a_guard_active") > 0 for row in rows),
        "a_loop_gap_over_2ms_delta": number(rows[-1], "a_loop_gap_over_2ms") - number(rows[0], "a_loop_gap_over_2ms"),
    }


def summarize_events(path: Path):
    header, rows, bad_rows = read_csv(path)
    return {
        "file": path.name,
        "rows": len(rows),
        "columns": len(header),
        "bad_rows": bad_rows,
        "duplicate_rows": len(rows) - len({tuple(row.items()) for row in rows}),
        "event_counts": Counter(row["event"] for row in rows),
        "severity_counts": Counter(row["severity"] for row in rows),
        "bus_related": [
            {
                "sequence": int(row["sequence"]),
                "time": row["wall_time_first"],
                "event": row["event"],
                "tec": int(row["tec"]),
                "rec": int(row["rec"]),
                "detail": row["detail_text"],
            }
            for row in rows
            if row["event"] in {"BUS_ERR", "BUS_OFF", "RECOVERY_SOFT", "RECOVERY_OK", "RECOVERY_FAIL"}
        ],
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--timeseries", type=Path, action="append", required=True)
    parser.add_argument("--events", type=Path, action="append", required=True)
    args = parser.parse_args()
    result = {
        "timeseries": [summarize_timeseries(path) for path in args.timeseries],
        "events": [summarize_events(path) for path in args.events],
    }
    print(json.dumps(result, ensure_ascii=False, indent=2, default=dict))


if __name__ == "__main__":
    main()
