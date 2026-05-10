#!/usr/bin/env python3
# T-CAN Explorer JSON에서 CAN 신호 상세 정보를 추출하는 CLI 도구
from __future__ import annotations

import argparse
import html
import json
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any
from urllib.parse import urlencode
from urllib.request import Request, urlopen


DEFAULT_BASE_URL = "https://tcan.latency.is"
DEFAULT_PLATFORM = "ModelY"
DEFAULT_BUS = "VEH,CH"


@dataclass(frozen=True)
class Match:
    signal_name: str
    frame: dict[str, Any]
    signal: dict[str, Any]
    matched_frame_sources: list[str]
    matched_signal_sources: list[str]
    tcan_url: str


def parse_csv(values: list[str] | None, default: str) -> list[str]:
    raw_values = values if values else [default]
    items: list[str] = []
    for raw_value in raw_values:
        for part in raw_value.split(","):
            item = part.strip()
            if item:
                items.append(item)
    return list(dict.fromkeys(items))


def fetch_json(base_url: str, path: str, timeout: float) -> Any:
    url = base_url.rstrip("/") + path
    request = Request(url, headers={"User-Agent": "tesla-open-can-mod tcan signal detail extractor"})
    with urlopen(request, timeout=timeout) as response:
        charset = response.headers.get_content_charset() or "utf-8"
        return json.loads(response.read().decode(charset))


def sanitize_frame_key(key: str) -> str:
    return "".join(char if char.isalnum() or char == "_" else "_" for char in key)


def source_matches(source_id: str, sources: dict[str, dict[str, Any]], platforms: set[str], buses: set[str]) -> bool:
    source = sources.get(source_id)
    if not source:
        return False
    return source.get("platform") in platforms and source.get("bus") in buses


def matching_sources(source_ids: list[str], sources: dict[str, dict[str, Any]], platforms: set[str], buses: set[str]) -> list[str]:
    return [source_id for source_id in source_ids if source_matches(source_id, sources, platforms, buses)]


def signal_bit_positions(layout: dict[str, Any]) -> list[int]:
    start_bit = int(layout["startBit"])
    length = int(layout["length"])
    byte_order = layout.get("byteOrder")
    if length <= 0:
        return []
    if byte_order == "little":
        return [start_bit + offset for offset in range(length)]

    positions: list[int] = []
    bit = start_bit
    for _ in range(length):
        positions.append(bit)
        bit = bit + 15 if bit % 8 == 0 else bit - 1
    return positions


def signal_bit_mapping(layout: dict[str, Any]) -> dict[int, int]:
    positions = signal_bit_positions(layout)
    length = int(layout["length"])
    mapping: dict[int, int] = {}
    for order_index, position in enumerate(positions):
        raw_bit = order_index if layout.get("byteOrder") == "little" else length - 1 - order_index
        mapping[position] = raw_bit
    return mapping


def bit_layout_rows(layout: dict[str, Any]) -> list[dict[str, Any]]:
    mapping = signal_bit_mapping(layout)
    if not mapping:
        return []
    bytes_used = sorted({position // 8 for position in mapping})
    rows: list[dict[str, Any]] = []
    for byte_index in range(bytes_used[0], bytes_used[-1] + 1):
        bits: list[dict[str, int | None]] = []
        for bit_index in range(7, -1, -1):
            absolute_bit = byte_index * 8 + bit_index
            bits.append(
                {
                    "bit": bit_index,
                    "absoluteBit": absolute_bit,
                    "rawBit": mapping.get(absolute_bit),
                }
            )
        rows.append({"byte": byte_index, "bits": bits})
    return rows


def bit_layout_markdown(layout: dict[str, Any]) -> list[str]:
    rows = bit_layout_rows(layout)
    if not rows:
        return ["No bits used."]
    lines = [
        "| Byte | bit7 | bit6 | bit5 | bit4 | bit3 | bit2 | bit1 | bit0 |",
        "|------|------|------|------|------|------|------|------|------|",
    ]
    for row in rows:
        cells = [f"raw[{bit['rawBit']}]" if bit["rawBit"] is not None else "." for bit in row["bits"]]
        lines.append(f"| `{row['byte']}` | " + " | ".join(f"`{cell}`" for cell in cells) + " |")
    return lines


def bit_layout_html(layout: dict[str, Any]) -> str:
    rows = bit_layout_rows(layout)
    if not rows:
        return '<p class="empty">No bits used.</p>'
    lines = [
        '<table class="bit-map">',
        '<thead><tr><th>byte</th><th>bit7</th><th>bit6</th><th>bit5</th><th>bit4</th><th>bit3</th><th>bit2</th><th>bit1</th><th>bit0</th></tr></thead>',
        '<tbody>',
    ]
    for row in rows:
        lines.append(f'<tr><th class="byte">byte {row["byte"]}</th>')
        for bit in row["bits"]:
            absolute_bit = bit["absoluteBit"]
            raw_bit = bit["rawBit"]
            if raw_bit is None:
                lines.append(
                    f'<td class="unused"><span class="raw">.</span><span class="abs">payload {absolute_bit}</span></td>'
                )
            else:
                lines.append(
                    '<td class="used">'
                    f'<span class="raw">raw[{raw_bit}]</span>'
                    f'<span class="abs">payload {absolute_bit}</span>'
                    '</td>'
                )
        lines.append('</tr>')
    lines.extend(['</tbody>', '</table>'])
    return "\n".join(lines)


def bit_layout_diagram(layout: dict[str, Any]) -> list[str]:
    rows = bit_layout_rows(layout)
    if not rows:
        return ["No bits used."]
    table_rows = [["byte", "bit7", "bit6", "bit5", "bit4", "bit3", "bit2", "bit1", "bit0"]]
    for row in rows:
        cells = [f"raw[{bit['rawBit']}]" if bit["rawBit"] is not None else "." for bit in row["bits"]]
        table_rows.append([f"byte {row['byte']}", *cells])
    widths = [max(len(row[column]) for row in table_rows) for column in range(len(table_rows[0]))]
    return ["  ".join(cell.rjust(widths[index]) for index, cell in enumerate(row)) for row in table_rows]


def byte_span(layout: dict[str, Any]) -> str:
    bytes_used = sorted({position // 8 for position in signal_bit_positions(layout)})
    if not bytes_used:
        return "n/a"
    if len(bytes_used) == 1:
        return f"byte {bytes_used[0]}"
    compact_ranges: list[str] = []
    range_start = bytes_used[0]
    previous = bytes_used[0]
    for byte_index in bytes_used[1:]:
        if byte_index == previous + 1:
            previous = byte_index
            continue
        compact_ranges.append(format_byte_range(range_start, previous))
        range_start = previous = byte_index
    compact_ranges.append(format_byte_range(range_start, previous))
    return ", ".join(compact_ranges)


def format_number(value: Any) -> str:
    if isinstance(value, bool):
        return str(value)
    if isinstance(value, int):
        return str(value)
    if isinstance(value, float):
        return f"{value:g}"
    return str(value)


def html_escape(value: Any) -> str:
    return html.escape(str(value), quote=True)


def html_id(value: str) -> str:
    safe = "".join(char if char.isalnum() else "-" for char in value).strip("-")
    return safe or "signal"


def html_join(values: list[Any]) -> str:
    return ", ".join(html_escape(value) for value in values)


def html_kv_grid(items: list[tuple[str, Any]]) -> str:
    lines = ['<dl class="kv-grid">']
    for label, value in items:
        lines.append(
            f'<div><dt>{html_escape(label)}</dt><dd>{html_escape(format_number(value))}</dd></div>'
        )
    lines.append('</dl>')
    return "\n".join(lines)


def grouped_matches_by_frame(matches: list[Match]) -> list[tuple[dict[str, Any], list[Match]]]:
    grouped: dict[str, tuple[dict[str, Any], list[Match]]] = {}
    order: list[str] = []
    for match in matches:
        frame_key = str(match.frame.get("key"))
        if frame_key not in grouped:
            grouped[frame_key] = (match.frame, [])
            order.append(frame_key)
        grouped[frame_key][1].append(match)
    return [grouped[frame_key] for frame_key in order]


def frame_bit_usage(matches: list[Match]) -> dict[int, list[str]]:
    usage: dict[int, list[str]] = {}
    for match in matches:
        signal_name = str(match.signal.get("name") or match.signal_name)
        layout = match.signal.get("layout", {})
        for absolute_bit in signal_bit_positions(layout):
            usage.setdefault(absolute_bit, []).append(signal_name)
    return usage


def frame_overlap_html(matches: list[Match], dlc: int) -> str:
    usage = frame_bit_usage(matches)
    if not usage:
        return '<p class="empty">No overlap information available.</p>'
    max_bit = max(min(dlc * 8, 64), max(usage) + 1)
    lines = [
        '<table class="bit-map overlap-map">',
        '<thead><tr><th>byte</th><th>bit7</th><th>bit6</th><th>bit5</th><th>bit4</th><th>bit3</th><th>bit2</th><th>bit1</th><th>bit0</th></tr></thead>',
        '<tbody>',
    ]
    for byte_index in range(max_bit // 8):
        lines.append(f'<tr><th class="byte">byte {byte_index}</th>')
        for bit_index in range(7, -1, -1):
            absolute_bit = byte_index * 8 + bit_index
            if absolute_bit >= max_bit:
                lines.append('<td class="unused"><span class="raw">.</span><span class="abs">out</span></td>')
                continue
            owners = usage.get(absolute_bit, [])
            if not owners:
                lines.append(
                    f'<td class="unused"><span class="raw">.</span><span class="abs">payload {absolute_bit}</span></td>'
                )
                continue
            owners_text = html_escape(", ".join(sorted(set(owners))))
            if len(owners) > 1:
                lines.append(
                    '<td class="overlap"'
                    f' title="Shared by: {owners_text}">'
                    f'<span class="raw">{len(owners)} signals</span>'
                    f'<span class="abs">payload {absolute_bit}</span>'
                    '</td>'
                )
            else:
                lines.append(
                    '<td class="used"'
                    f' title="Used by: {owners_text}">'
                    f'<span class="raw">{html_escape(owners[0])}</span>'
                    f'<span class="abs">payload {absolute_bit}</span>'
                    '</td>'
                )
        lines.append('</tr>')
    lines.extend(['</tbody>', '</table>'])
    return "\n".join(lines)


def format_byte_range(start: int, end: int) -> str:
    if start == end:
        return f"byte {start}"
    return f"bytes {start}..{end}"


def dbc_signal_line(signal: dict[str, Any]) -> str:
    layout = signal["layout"]
    byte_order = "1" if layout.get("byteOrder") == "little" else "0"
    sign = "-" if layout.get("signed") else "+"
    unit = layout.get("unit") or ""
    return (
        f"SG_ {signal['name']} : {layout['startBit']}|{layout['length']}@{byte_order}{sign} "
        f"({layout['scale']},{layout['offset']}) [{layout['min']}|{layout['max']}] \"{unit}\" Vector__XXX"
    )


def physical_formula(layout: dict[str, Any]) -> str:
    scale = layout.get("scale", 1)
    offset = layout.get("offset", 0)
    if offset == 0:
        return f"physical = raw * {format_number(scale)}"
    sign = "+" if offset > 0 else "-"
    return f"physical = raw * {format_number(scale)} {sign} {format_number(abs(offset))}"


def raw_mask(layout: dict[str, Any]) -> int:
    length = int(layout["length"])
    return (1 << length) - 1 if length > 0 else 0


def physical_range(layout: dict[str, Any]) -> tuple[float, float]:
    scale = float(layout.get("scale", 1))
    offset = float(layout.get("offset", 0))
    raw_max = raw_mask(layout)
    first = offset
    second = raw_max * scale + offset
    return (min(first, second), max(first, second))


def extraction_pseudocode(layout: dict[str, Any]) -> list[str]:
    mask = raw_mask(layout)
    if layout.get("byteOrder") == "little":
        return [
            "payload_le = data[0] | (data[1] << 8) | ... | (data[7] << 56)",
            f"raw = (payload_le >> {layout['startBit']}) & 0x{mask:X}",
        ]

    positions = signal_bit_positions(layout)
    return [
        "raw = 0",
        f"for bit_position in {positions}:",
        "    raw = (raw << 1) | ((data[bit_position // 8] >> (bit_position % 8)) & 1)",
    ]


def signal_derived(signal: dict[str, Any]) -> dict[str, Any]:
    layout = signal["layout"]
    raw_max = raw_mask(layout)
    physical_min, physical_max = physical_range(layout)
    return {
        "byteSpan": byte_span(layout),
        "bitPositions": signal_bit_positions(layout),
        "bitLayout": bit_layout_rows(layout),
        "rawMask": f"0x{raw_max:X}",
        "rawRange": [0, raw_max],
        "physicalRange": [physical_min, physical_max],
        "physicalFormula": physical_formula(layout),
        "dbcSignalLine": dbc_signal_line(signal),
        "extractionPseudocode": extraction_pseudocode(layout),
    }


def frame_url(base_url: str, platforms: list[str], buses: list[str], frame_key: str) -> str:
    query = urlencode({"plat": ",".join(platforms), "bus": ",".join(buses), "frame": frame_key})
    return f"{base_url.rstrip('/')}/?{query}"


def find_matches(
    manifest: dict[str, Any],
    base_url: str,
    signal_names: list[str],
    platforms: list[str],
    buses: list[str],
    timeout: float,
) -> list[Match]:
    sources = {source["id"]: source for source in manifest.get("sources", [])}
    platform_set = set(platforms)
    bus_set = set(buses)
    requested = set(signal_names)
    matches: list[Match] = []

    for frame_summary in manifest.get("frames", []):
        frame_signal_names = set(frame_summary.get("signalNames", []))
        if not frame_signal_names.intersection(requested):
            continue
        if not set(frame_summary.get("buses", [])).intersection(bus_set):
            continue

        matched_frame_sources = matching_sources(frame_summary.get("presentIn", []), sources, platform_set, bus_set)
        if not matched_frame_sources:
            continue

        frame_key = frame_summary["key"]
        frame_detail = fetch_json(base_url, f"/data/frames/{sanitize_frame_key(frame_key)}.json", timeout)
        matched_signal_by_name = {
            signal["name"]: signal
            for signal in frame_detail.get("signals", [])
            if signal.get("name") in requested
        }
        for signal_name in signal_names:
            signal = matched_signal_by_name.get(signal_name)
            if not signal:
                continue
            matched_signal_sources = matching_sources(signal.get("presentIn", []), sources, platform_set, bus_set)
            if not matched_signal_sources:
                continue
            matches.append(
                Match(
                    signal_name=signal_name,
                    frame=frame_detail,
                    signal=signal,
                    matched_frame_sources=matched_frame_sources,
                    matched_signal_sources=matched_signal_sources,
                    tcan_url=frame_url(base_url, platforms, buses, frame_key),
                )
            )

    return matches


def render_markdown(
    matches: list[Match],
    missing_signals: list[str],
    base_url: str,
    platforms: list[str],
    buses: list[str],
    signal_names: list[str],
) -> str:
    generated_at = datetime.now(timezone.utc).astimezone().isoformat(timespec="seconds")
    lines = [
        "# T-CAN Signal Detail Reference",
        "",
        f"Generated at: `{generated_at}`",
        f"Source: `{base_url.rstrip('/')}/data/manifest.json`",
        f"Filter: platform `{', '.join(platforms)}`, bus `{', '.join(buses)}`",
        f"Requested signals: `{', '.join(signal_names)}`",
        "",
        "이 문서는 T-CAN Explorer의 공개 JSON 데이터를 기준으로 생성했다.",
        "Derived DBC line과 byte span은 읽기 편의를 위한 보조 정보다.",
        "",
    ]

    matches_by_signal: dict[str, list[Match]] = {signal_name: [] for signal_name in signal_names}
    for match in matches:
        matches_by_signal.setdefault(match.signal_name, []).append(match)

    for signal_name in signal_names:
        lines.extend([f"## {signal_name}", ""])
        signal_matches = matches_by_signal.get(signal_name, [])
        if not signal_matches:
            lines.extend(["No matching signal found for the selected filters.", ""])
            continue
        for match in signal_matches:
            frame = match.frame
            signal = match.signal
            layout = signal["layout"]
            derived = signal_derived(signal)
            lines.extend(
                [
                    f"### {frame['addressHex']} {frame['canonicalName']}",
                    "",
                    f"- T-CAN URL: {match.tcan_url}",
                    f"- Frame key: `{frame['key']}`",
                    f"- Address: decimal `{frame['address']}`, hex `{frame['addressHex']}`",
                    f"- Frame name: `{frame['canonicalName']}`",
                    f"- Module: `{frame.get('module', '')}`",
                    f"- Buses in T-CAN: `{', '.join(frame.get('buses', []))}`",
                    f"- DLC: `{frame.get('length')}` bytes",
                    f"- Frame signal count: `{frame.get('signalCount')}`",
                    f"- Matching frame sources: `{', '.join(match.matched_frame_sources)}`",
                    f"- Matching signal sources: `{', '.join(match.matched_signal_sources)}`",
                    f"- All frame sources: `{', '.join(frame.get('presentIn', []))}`",
                    f"- All signal sources: `{', '.join(signal.get('presentIn', []))}`",
                    "",
                    "Signal layout.",
                    "",
                    f"- Start bit: `{layout['startBit']}`",
                    f"- Length: `{layout['length']}` bits",
                    f"- Byte order: `{layout['byteOrder']}`",
                    f"- Signed: `{layout['signed']}`",
                    f"- Byte span: `{derived['byteSpan']}`",
                    f"- Bit positions: `{derived['bitPositions']}`",
                    f"- Scale: `{layout['scale']}`",
                    f"- Offset: `{layout['offset']}`",
                    f"- Min/Max from source: `{layout['min']}` / `{layout['max']}`",
                    f"- Raw mask: `{derived['rawMask']}`",
                    f"- Raw range by bit width: `{derived['rawRange'][0]}` / `{derived['rawRange'][1]}`",
                    f"- Physical range by bit width: `{format_number(derived['physicalRange'][0])}` / `{format_number(derived['physicalRange'][1])}`",
                    f"- Unit: `{layout.get('unit') or ''}`",
                    f"- Mux: `{layout.get('mux')}`",
                    f"- Physical formula: `{derived['physicalFormula']}`",
                    "",
                    "Visual bit map.",
                    "",
                    "`raw[0]` is the least significant bit of the extracted raw value. Columns are payload bit positions inside each byte.",
                    "",
                    "```text",
                    *bit_layout_diagram(layout),
                    "```",
                    "",
                    "Bit layout table.",
                    "",
                    *bit_layout_markdown(layout),
                    "",
                    "DBC-style signal line.",
                    "",
                    "```dbc",
                    derived["dbcSignalLine"],
                    "```",
                    "",
                    "Raw extraction pseudocode.",
                    "",
                    "```text",
                    *derived["extractionPseudocode"],
                    "```",
                    "",
                ]
            )
            if signal.get("vapiAlias") or signal.get("vapiSource"):
                lines.extend(
                    [
                        "VAPI metadata.",
                        "",
                        f"- Alias: `{signal.get('vapiAlias', '')}`",
                        f"- Source: `{signal.get('vapiSource', '')}`",
                        "",
                    ]
                )
            enum_map = signal.get("enumMap") or []
            if enum_map:
                lines.extend([f"Enum symbol: `{signal.get('enumSymbol', '')}`", "", "| Dec | Hex | Label |", "|-----|-----|-------|"])
                for item in enum_map:
                    lines.append(f"| `{item.get('value')}` | `{item.get('hex')}` | `{item.get('label', '')}` |")
                lines.append("")
            else:
                lines.extend(["Enum map: not present in T-CAN data.", ""])

    if missing_signals:
        lines.extend(["## Missing Signals", ""])
        for signal_name in missing_signals:
            lines.append(f"- `{signal_name}`")
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def render_json(
    matches: list[Match],
    missing_signals: list[str],
    base_url: str,
    platforms: list[str],
    buses: list[str],
    signal_names: list[str],
) -> str:
    payload = {
        "generatedAt": datetime.now(timezone.utc).astimezone().isoformat(timespec="seconds"),
        "source": base_url.rstrip("/") + "/data/manifest.json",
        "filters": {"platforms": platforms, "buses": buses},
        "requestedSignals": signal_names,
        "missingSignals": missing_signals,
        "matches": [
            {
                "signalName": match.signal_name,
                "tcanUrl": match.tcan_url,
                "matchedFrameSources": match.matched_frame_sources,
                "matchedSignalSources": match.matched_signal_sources,
                "frame": {
                    "key": match.frame.get("key"),
                    "address": match.frame.get("address"),
                    "addressHex": match.frame.get("addressHex"),
                    "canonicalName": match.frame.get("canonicalName"),
                    "module": match.frame.get("module"),
                    "buses": match.frame.get("buses", []),
                    "length": match.frame.get("length"),
                    "presentIn": match.frame.get("presentIn", []),
                },
                "signal": match.signal,
                "derived": signal_derived(match.signal),
            }
            for match in matches
        ],
    }
    return json.dumps(payload, indent=2, ensure_ascii=False) + "\n"


def render_html(
    matches: list[Match],
    missing_signals: list[str],
    base_url: str,
    platforms: list[str],
    buses: list[str],
    signal_names: list[str],
) -> str:
    generated_at = datetime.now(timezone.utc).astimezone().isoformat(timespec="seconds")
    frame_groups = grouped_matches_by_frame(matches)

    lines = [
        '<!doctype html>',
        '<html lang="en">',
        '<head>',
        '<meta charset="utf-8">',
        '<meta name="viewport" content="width=device-width, initial-scale=1">',
        '<title>T-CAN Signal Detail Reference</title>',
        '<style>',
        ':root{color-scheme:light dark;--bg:#f6f8fb;--panel:#ffffff;--text:#1b2430;--muted:#667085;--line:#d6dee8;--accent:#0b6bcb;--accent2:#0f8f72;--warn:#b45309;--code:#111827;--used:#dff4ec;--used-border:#79c7aa;--unused:#f3f5f8}',
        '@media (prefers-color-scheme:dark){:root{--bg:#0d1117;--panel:#161b22;--text:#e6edf3;--muted:#8b949e;--line:#30363d;--accent:#58a6ff;--accent2:#56d6a6;--warn:#f0b45b;--code:#0b1018;--used:#113a31;--used-border:#2ea77e;--unused:#1f2630}}',
        '*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:14px/1.55 -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif}main{max-width:1180px;margin:0 auto;padding:28px 18px 56px}h1{font-size:30px;margin:0 0 8px}h2{font-size:24px;margin:0 0 12px}h3{font-size:18px;margin:0}h4{font-size:14px;margin:22px 0 8px;color:var(--muted);text-transform:uppercase;letter-spacing:.04em}.intro{color:var(--muted);margin:0 0 18px}.chips{display:flex;flex-wrap:wrap;gap:8px;margin:14px 0 22px}.chip{border:1px solid var(--line);background:var(--panel);border-radius:6px;padding:6px 9px;color:var(--muted)}nav{display:flex;flex-wrap:wrap;gap:8px;margin:18px 0 28px}nav a{color:var(--accent);border:1px solid var(--line);background:var(--panel);padding:7px 10px;border-radius:6px;text-decoration:none}.legend{position:sticky;top:8px;z-index:6;display:flex;flex-wrap:wrap;gap:10px;margin:16px 0 24px;padding:8px;background:color-mix(in srgb,var(--bg) 70%,transparent)}.legend .item{display:flex;align-items:center;gap:8px;border:1px solid var(--line);background:var(--panel);border-radius:8px;padding:8px 10px;color:var(--muted)}.legend .swatch{width:18px;height:18px;border-radius:4px;border:1px solid var(--line)}.legend .used{background:var(--used);border-color:var(--used-border)}.legend .unused{background:var(--unused)}.legend .byte{background:color-mix(in srgb,var(--panel) 80%,var(--accent) 20%)}.legend .overlap{background:color-mix(in srgb,var(--warn) 20%,var(--panel) 80%);border-color:color-mix(in srgb,var(--warn) 60%,var(--line) 40%)}.frame-card{border:1px solid var(--line);background:var(--panel);border-radius:10px;padding:18px;margin:18px 0 24px;box-shadow:0 1px 2px rgba(16,24,40,.05)}.frame-head{display:flex;gap:12px;justify-content:space-between;align-items:flex-start;margin-bottom:14px}.frame-head a{color:var(--accent);text-decoration:none;word-break:break-all}.frame-signals{display:flex;flex-wrap:wrap;gap:8px;margin:10px 0 16px}.frame-signals .sig{border:1px solid var(--line);background:color-mix(in srgb,var(--panel) 80%,var(--bg) 20%);border-radius:999px;padding:5px 9px;font-size:12px;color:var(--text)}.signal-card{border:1px solid var(--line);background:color-mix(in srgb,var(--panel) 90%,var(--bg) 10%);border-radius:8px;padding:18px;margin:12px 0 18px}.card-head{display:flex;gap:12px;justify-content:space-between;align-items:flex-start;margin-bottom:14px}.card-head a{color:var(--accent);text-decoration:none;word-break:break-all}.kv-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:9px;margin:10px 0 16px}.kv-grid div{border:1px solid var(--line);border-radius:6px;padding:9px;background:color-mix(in srgb,var(--panel) 88%,var(--bg))}.kv-grid dt{font-size:11px;color:var(--muted);margin-bottom:3px}.kv-grid dd{margin:0;font-weight:650;word-break:break-word}.note{color:var(--muted);margin:6px 0 10px}.bit-wrap{overflow-x:auto;border:1px solid var(--line);border-radius:8px}.bit-map{width:100%;min-width:760px;border-collapse:separate;border-spacing:0}.bit-map th,.bit-map td{border-right:1px solid var(--line);border-bottom:1px solid var(--line);padding:8px;text-align:center;vertical-align:middle}.bit-map thead th{position:sticky;top:0;background:var(--code);color:#f8fafc;font-weight:750;z-index:2}.bit-map thead th:first-child{left:0;z-index:4}.bit-map tr:last-child td,.bit-map tr:last-child th{border-bottom:0}.bit-map th:last-child,.bit-map td:last-child{border-right:0}.bit-map .byte{position:sticky;left:0;z-index:1;background:color-mix(in srgb,var(--panel) 80%,var(--accent) 20%);font-weight:800;white-space:nowrap}.bit-map td.used{background:var(--used);border-color:var(--used-border)}.bit-map td.unused{background:var(--unused);color:var(--muted)}.bit-map td.overlap{background:color-mix(in srgb,var(--warn) 26%,var(--panel) 74%);border-color:color-mix(in srgb,var(--warn) 50%,var(--line) 50%)}.bit-map .raw{display:block;font-family:ui-monospace,SFMono-Regular,Menlo,monospace;font-weight:800}.bit-map .abs{display:block;font-size:11px;color:var(--muted);margin-top:2px}.used .raw{color:var(--accent2)}.overlap .raw{color:color-mix(in srgb,var(--warn) 70%,var(--text) 30%)}pre{background:var(--code);color:#f8fafc;border-radius:8px;padding:12px;overflow-x:auto;white-space:pre;font:12px/1.5 ui-monospace,SFMono-Regular,Menlo,monospace}.enum{width:100%;border-collapse:collapse}.enum th,.enum td{border:1px solid var(--line);padding:7px 8px;text-align:left}.empty{color:var(--warn);font-weight:700}.missing{border:1px solid var(--line);background:var(--panel);border-radius:8px;padding:14px}',
        '</style>',
        '</head>',
        '<body><main>',
        '<h1>T-CAN Signal Detail Reference</h1>',
        '<p class="intro">Generated from T-CAN Explorer public JSON. Markdown remains the default output; this HTML view is optimized for visual bit layout inspection.</p>',
        '<div class="chips">',
        f'<span class="chip">Generated: {html_escape(generated_at)}</span>',
        f'<span class="chip">Source: {html_escape(base_url.rstrip("/") + "/data/manifest.json")}</span>',
        f'<span class="chip">Platform: {html_join(platforms)}</span>',
        f'<span class="chip">Bus: {html_join(buses)}</span>',
        '</div>',
        '<nav>',
    ]
    for frame, frame_matches in frame_groups:
        lines.append(
            f'<a href="#{html_id(str(frame.get("key")))}">{html_escape(str(frame.get("addressHex")))} {html_escape(str(frame.get("canonicalName")))}</a>'
        )
    lines.extend(
        [
            '</nav>',
            '<div class="legend">',
            '<div class="item"><span class="swatch used"></span><span>Used bit mapped to extracted raw value</span></div>',
            '<div class="item"><span class="swatch unused"></span><span>Unused payload bit inside displayed byte span</span></div>',
            '<div class="item"><span class="swatch byte"></span><span>Sticky byte index column</span></div>',
            '<div class="item"><span class="swatch overlap"></span><span>Overlapping payload bit used by multiple selected signals</span></div>',
            '</div>',
        ]
    )

    if not frame_groups:
        lines.append('<p class="empty">No matching signal found for the selected filters.</p>')

    for frame, frame_matches in frame_groups:
        lines.extend(
            [
                f'<section id="{html_id(str(frame.get("key")))}" class="frame-card">',
                '<div class="frame-head">',
                f'<div><h2>{html_escape(frame["addressHex"])} {html_escape(frame["canonicalName"])}</h2><p class="intro">Frame-level group for visually comparing multiple signals that share the same payload.</p></div>',
                f'<a href="{html_escape(frame_matches[0].tcan_url)}">Open in T-CAN</a>',
                '</div>',
                '<div class="frame-signals">',
            ]
        )
        for match in frame_matches:
            lines.append(f'<span class="sig">{html_escape(match.signal_name)}</span>')
        lines.extend(
            [
                '</div>',
                html_kv_grid(
                    [
                        ("Frame key", frame["key"]),
                        ("Address", f'{frame["address"]} / {frame["addressHex"]}'),
                        ("Module", frame.get("module", "")),
                        ("Buses", ", ".join(frame.get("buses", []))),
                        ("DLC", f'{frame.get("length")} bytes'),
                        ("Signal count", frame.get("signalCount")),
                        ("Matching frame sources", ", ".join(frame_matches[0].matched_frame_sources)),
                    ]
                ),
                '<h4>Frame Bit Overlap Map</h4>',
                '<p class="note">Cells in overlap color are payload bits shared by two or more selected signals in this frame.</p>',
                '<div class="bit-wrap">',
                frame_overlap_html(frame_matches, int(frame.get("length") or 8)),
                '</div>',
            ]
        )
        for match in frame_matches:
            frame = match.frame
            signal = match.signal
            layout = signal["layout"]
            derived = signal_derived(signal)
            lines.extend(
                [
                    '<article class="signal-card">',
                    '<div class="card-head">',
                    f'<h3>{html_escape(match.signal_name)}</h3>',
                    f'<a href="{html_escape(match.tcan_url)}">Signal in T-CAN</a>',
                    '</div>',
                    '<h4>Signal Layout</h4>',
                    html_kv_grid(
                        [
                            ("Start bit", layout["startBit"]),
                            ("Length", f'{layout["length"]} bits'),
                            ("Byte order", layout["byteOrder"]),
                            ("Signed", layout["signed"]),
                            ("Byte span", derived["byteSpan"]),
                            ("Bit positions", derived["bitPositions"]),
                            ("Scale", layout["scale"]),
                            ("Offset", layout["offset"]),
                            ("Raw mask", derived["rawMask"]),
                            ("Raw range", f'{derived["rawRange"][0]} / {derived["rawRange"][1]}'),
                            ("Physical range", f'{format_number(derived["physicalRange"][0])} / {format_number(derived["physicalRange"][1])}'),
                            ("Matching signal sources", ", ".join(match.matched_signal_sources)),
                            ("Formula", derived["physicalFormula"]),
                            ("Unit", layout.get("unit") or ""),
                            ("Mux", layout.get("mux")),
                        ]
                    ),
                    '<h4>Bit Layout</h4>',
                    '<p class="note"><code>raw[0]</code> is the least significant bit of the extracted raw value. Each used cell also shows the absolute payload bit.</p>',
                    '<div class="bit-wrap">',
                    bit_layout_html(layout),
                    '</div>',
                    '<h4>DBC-style Signal Line</h4>',
                    f'<pre>{html_escape(derived["dbcSignalLine"])}</pre>',
                    '<h4>Raw Extraction Pseudocode</h4>',
                    f'<pre>{html_escape(chr(10).join(derived["extractionPseudocode"]))}</pre>',
                ]
            )
            if signal.get("vapiAlias") or signal.get("vapiSource"):
                lines.extend(
                    [
                        '<h4>VAPI Metadata</h4>',
                        html_kv_grid([("Alias", signal.get("vapiAlias", "")), ("Source", signal.get("vapiSource", ""))]),
                    ]
                )
            enum_map = signal.get("enumMap") or []
            if enum_map:
                lines.extend(['<h4>Enum Map</h4>', '<table class="enum"><thead><tr><th>Dec</th><th>Hex</th><th>Label</th></tr></thead><tbody>'])
                for item in enum_map:
                    lines.append(
                        '<tr>'
                        f'<td>{html_escape(item.get("value"))}</td>'
                        f'<td>{html_escape(item.get("hex"))}</td>'
                        f'<td>{html_escape(item.get("label", ""))}</td>'
                        '</tr>'
                    )
                lines.append('</tbody></table>')
            else:
                lines.append('<p class="note">Enum map: not present in T-CAN data.</p>')
            lines.append('</article>')
        lines.append('</section>')

    if missing_signals:
        lines.extend(['<section class="missing"><h2>Missing Signals</h2><ul>'])
        for signal_name in missing_signals:
            lines.append(f'<li>{html_escape(signal_name)}</li>')
        lines.extend(['</ul></section>'])

    lines.extend(['</main></body>', '</html>'])
    return "\n".join(lines) + "\n"


def write_or_print(text: str, output: Path | None) -> None:
    if output is None:
        print(text, end="")
        return
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text, encoding="utf-8")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Extract detailed signal information from T-CAN Explorer data.")
    parser.add_argument("signals", nargs="*", help="Signal names to extract. Comma-separated values are also accepted.")
    parser.add_argument("--base-url", default=DEFAULT_BASE_URL, help=f"T-CAN base URL. Default: {DEFAULT_BASE_URL}")
    parser.add_argument("--platform", action="append", help=f"Platform filter. Default: {DEFAULT_PLATFORM}")
    parser.add_argument("--bus", action="append", help=f"Bus filter. Default: {DEFAULT_BUS}")
    parser.add_argument("--format", choices=("markdown", "md", "html", "json"), default="markdown", help="Output format. Default: markdown (.md).")
    parser.add_argument("--html", action="store_true", help="Shortcut for --format html.")
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
    manifest = fetch_json(args.base_url, "/data/manifest.json", args.timeout)
    matches = find_matches(manifest, args.base_url, signal_names, platforms, buses, args.timeout)
    matched_names = {match.signal_name for match in matches}
    missing_signals = [signal_name for signal_name in signal_names if signal_name not in matched_names]

    output_format = "html" if args.html else args.format
    if output_format == "md":
        output_format = "markdown"

    if output_format == "json":
        text = render_json(matches, missing_signals, args.base_url, platforms, buses, signal_names)
    elif output_format == "html":
        text = render_html(matches, missing_signals, args.base_url, platforms, buses, signal_names)
    else:
        text = render_markdown(matches, missing_signals, args.base_url, platforms, buses, signal_names)

    write_or_print(text, args.output)
    return 1 if missing_signals else 0


if __name__ == "__main__":
    raise SystemExit(main())