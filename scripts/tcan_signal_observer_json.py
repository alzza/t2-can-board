# T-CAN 신호 이름으로 신호 관찰기 JSON을 생성하는 CLI 도구
from __future__ import annotations

import argparse
import json
import re
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from tcan_signal_detail import (
    DEFAULT_BASE_URL,
    DEFAULT_BUS,
    DEFAULT_PLATFORM,
    fetch_json,
    find_matches,
    parse_csv,
)


OBSERVER_CHANNELS = ("auto", "A", "B", "A+B")
DEFAULT_MAX_SIGNALS = 10
DEFAULT_MAX_A_FILTER_IDS = 6
DEFAULT_A_BASE_IDS = (659, 1016, 1021)


def source_bus_tokens(source_ids: list[str]) -> set[str]:
    tokens: set[str] = set()
    for source_id in source_ids:
        tokens.update(re.findall(r"[A-Z0-9]+", source_id.upper()))
    return tokens


def infer_channel(source_ids: list[str], buses: list[str]) -> str:
    bus_set = {bus.upper() for bus in buses}
    if "VEH" in bus_set and "CH" in bus_set:
        return "A+B"
    if bus_set == {"CH"}:
        return "B"
    if bus_set == {"VEH"}:
        return "A"

    tokens = source_bus_tokens(source_ids)
    has_veh = "VEH" in tokens
    has_ch = "CH" in tokens
    if has_veh and has_ch:
        return "A+B"
    if has_ch:
        return "B"
    if has_veh:
        return "A"

    return "A+B"


def observer_entry(match: Any, channel: str, buses: list[str], idle: int) -> dict[str, Any]:
    layout = match.signal["layout"]
    byte_order = str(layout.get("byteOrder") or "").lower()
    if byte_order not in {"little", "big"}:
        raise ValueError(f"{match.signal_name}: unsupported byte order {layout.get('byteOrder')!r}")

    entry_channel = infer_channel(match.matched_signal_sources, buses) if channel == "auto" else channel
    return {
        "name": match.signal_name,
        "channel": entry_channel,
        "frame_id": int(match.frame["address"]),
        "frame_hex": match.frame.get("addressHex"),
        "start_bit": int(layout["startBit"]),
        "length": int(layout["length"]),
        "idle": idle,
        "byte_order": byte_order,
    }


def observer_uses_a_filter(entry: dict[str, Any]) -> bool:
    return entry["channel"] in {"A", "A+B"}


def validate_a_filter_budget(signals: list[dict[str, Any]], max_a_filter_ids: int, base_ids: tuple[int, ...]) -> None:
    ids = set(base_ids)
    for entry in signals:
        if observer_uses_a_filter(entry):
            ids.add(int(entry["frame_id"]))

    if len(ids) > max_a_filter_ids:
        observer_ids = sorted({int(entry["frame_id"]) for entry in signals if observer_uses_a_filter(entry)})
        raise ValueError(
            "A-channel filter ID limit exceeded: "
            f"base={','.join(hex(frame_id) for frame_id in base_ids)} "
            f"observer={','.join(hex(frame_id) for frame_id in observer_ids)} "
            f"total={len(ids)}/{max_a_filter_ids}. "
            f"Use B-only signals or reduce A/A+B unique frame IDs to {max_a_filter_ids - len(set(base_ids))}."
        )


def a_filter_ids_for(signals: list[dict[str, Any]], base_ids: tuple[int, ...]) -> list[int]:
    ids = set(base_ids)
    for entry in signals:
        if observer_uses_a_filter(entry):
            ids.add(int(entry["frame_id"]))
    return sorted(ids)


def render_payload(
    signal_names: list[str],
    base_url: str,
    platforms: list[str],
    buses: list[str],
    channel: str,
    idle: int,
    timeout: float,
    allow_missing: bool,
    max_signals: int,
    max_a_filter_ids: int,
) -> tuple[str, list[str]]:
    manifest = fetch_json(base_url, "/data/manifest.json", timeout)
    matches = find_matches(manifest, base_url, signal_names, platforms, buses, timeout)
    matched_names = {match.signal_name for match in matches}
    missing_signals = [signal_name for signal_name in signal_names if signal_name not in matched_names]
    if missing_signals and not allow_missing:
        raise ValueError("missing signals: " + ", ".join(missing_signals))
    if len(matches) > max_signals:
        raise ValueError(f"matched {len(matches)} signals, observer limit is {max_signals}")

    signals = [observer_entry(match, channel, buses, idle) for match in matches]
    validate_a_filter_budget(signals, max_a_filter_ids, DEFAULT_A_BASE_IDS)
    a_filter_ids = a_filter_ids_for(signals, DEFAULT_A_BASE_IDS)
    payload = {
        "signals": signals,
        "meta": {
            "generatedAt": datetime.now(timezone.utc).astimezone().isoformat(timespec="seconds"),
            "source": base_url.rstrip("/") + "/data/manifest.json",
            "platforms": platforms,
            "buses": buses,
            "channel": channel,
            "maxSignals": max_signals,
            "maxAFilterIds": max_a_filter_ids,
            "observerLimit": max_signals,
            "aFilterLimit": max_a_filter_ids,
            "aFilterBaseIds": list(DEFAULT_A_BASE_IDS),
            "aFilterIds": a_filter_ids,
            "aFilterUsed": len(a_filter_ids),
            "aFilterAdditionalCapacity": max_a_filter_ids - len(set(DEFAULT_A_BASE_IDS)),
            "aFilterRemaining": max_a_filter_ids - len(a_filter_ids),
            "missingSignals": missing_signals,
        },
    }
    return json.dumps(payload, indent=2, ensure_ascii=False) + "\n", missing_signals


def write_or_print(text: str, output: Path | None) -> None:
    if output is None:
        print(text, end="")
        return
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text, encoding="utf-8")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Generate signal observer JSON from T-CAN Explorer signal names.")
    parser.add_argument("signals", nargs="*", help="Signal names. Comma-separated values are also accepted.")
    parser.add_argument("--base-url", default=DEFAULT_BASE_URL, help=f"T-CAN base URL. Default: {DEFAULT_BASE_URL}")
    parser.add_argument("--platform", action="append", help=f"Platform filter. Default: {DEFAULT_PLATFORM}")
    parser.add_argument("--bus", action="append", help=f"Bus filter. Default: {DEFAULT_BUS}")
    parser.add_argument("--channel", choices=OBSERVER_CHANNELS, default="auto", help="Observer channel. auto maps VEH to A, CH to B, VEH+CH to A+B.")
    parser.add_argument("--idle", type=int, default=0, help="Raw idle value used for active/burst counting. Default: 0.")
    parser.add_argument("--max-signals", type=int, default=DEFAULT_MAX_SIGNALS, help=f"Observer slot limit. Default: {DEFAULT_MAX_SIGNALS}.")
    parser.add_argument("--max-a-filter-ids", type=int, default=DEFAULT_MAX_A_FILTER_IDS, help=f"A-channel MCP2515 filter ID limit including base IDs 659,1016,1021. Default: {DEFAULT_MAX_A_FILTER_IDS}.")
    parser.add_argument("--allow-missing", action="store_true", help="Write JSON even when some requested signals are missing.")
    parser.add_argument("--output", type=Path, help="Output file path. Prints to stdout when omitted.")
    parser.add_argument("--timeout", type=float, default=20.0, help="HTTP timeout in seconds.")
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    signal_names = parse_csv(args.signals, "")
    if not signal_names:
        parser.error("at least one signal name is required")

    platforms = parse_csv(args.platform, DEFAULT_PLATFORM)
    buses = parse_csv(args.bus, DEFAULT_BUS)
    try:
        text, missing_signals = render_payload(
            signal_names=signal_names,
            base_url=args.base_url,
            platforms=platforms,
            buses=buses,
            channel=args.channel,
            idle=args.idle,
            timeout=args.timeout,
            allow_missing=args.allow_missing,
            max_signals=args.max_signals,
            max_a_filter_ids=args.max_a_filter_ids,
        )
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    write_or_print(text, args.output)
    return 1 if missing_signals else 0


if __name__ == "__main__":
    raise SystemExit(main())
