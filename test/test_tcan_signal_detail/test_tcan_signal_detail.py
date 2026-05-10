# T-CAN 신호 bit layout 계산을 검증하는 Python unittest
from __future__ import annotations

import importlib.util
from pathlib import Path
import sys
import unittest


MODULE_PATH = Path(__file__).resolve().parents[2] / "scripts" / "tcan_signal_detail.py"
SPEC = importlib.util.spec_from_file_location("tcan_signal_detail", MODULE_PATH)
assert SPEC is not None and SPEC.loader is not None
TCAN = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = TCAN
SPEC.loader.exec_module(TCAN)


class TcanSignalDetailBitLayoutTest(unittest.TestCase):
    def test_little_endian_layout_maps_raw_bits_from_lsb(self) -> None:
        layout = {"startBit": 16, "length": 14, "byteOrder": "little"}

        mapping = TCAN.signal_bit_mapping(layout)
        diagram = TCAN.bit_layout_diagram(layout)

        self.assertEqual(mapping[16], 0)
        self.assertEqual(mapping[23], 7)
        self.assertEqual(mapping[24], 8)
        self.assertEqual(mapping[29], 13)
        self.assertIn("byte 2", diagram[1])
        self.assertIn("raw[0]", diagram[1])
        self.assertIn("raw[13]", diagram[2])

    def test_big_endian_layout_follows_motorola_bit_order(self) -> None:
        layout = {"startBit": 19, "length": 12, "byteOrder": "big"}

        positions = TCAN.signal_bit_positions(layout)
        mapping = TCAN.signal_bit_mapping(layout)
        diagram = TCAN.bit_layout_diagram(layout)

        self.assertEqual(positions, [19, 18, 17, 16, 31, 30, 29, 28, 27, 26, 25, 24])
        self.assertEqual(mapping[19], 11)
        self.assertEqual(mapping[16], 8)
        self.assertEqual(mapping[31], 7)
        self.assertEqual(mapping[24], 0)
        self.assertIn("raw[11]", diagram[1])
        self.assertIn("raw[0]", diagram[2])

    def test_markdown_table_keeps_byte_and_bit_headers(self) -> None:
        layout = {"startBit": 39, "length": 2, "byteOrder": "big"}

        table = TCAN.bit_layout_markdown(layout)

        self.assertEqual(table[0], "| Byte | bit7 | bit6 | bit5 | bit4 | bit3 | bit2 | bit1 | bit0 |")
        self.assertIn("| `4` | `raw[1]` | `raw[0]` |", table[2])

    def test_html_bit_layout_table_marks_used_and_unused_bits(self) -> None:
        layout = {"startBit": 19, "length": 12, "byteOrder": "big"}

        table = TCAN.bit_layout_html(layout)

        self.assertIn('<table class="bit-map">', table)
        self.assertIn('<td class="used"><span class="raw">raw[11]</span><span class="abs">payload 19</span></td>', table)
        self.assertIn('<td class="unused"><span class="raw">.</span><span class="abs">payload 23</span></td>', table)

    def test_parser_accepts_html_output_options(self) -> None:
        parser = TCAN.build_parser()

        args = parser.parse_args(["SCCM_steeringAngle", "--format", "html"])
        shortcut_args = parser.parse_args(["SCCM_steeringAngle", "--html"])

        self.assertEqual(args.format, "html")
        self.assertTrue(shortcut_args.html)

    def test_render_html_groups_signals_by_frame_and_shows_legend(self) -> None:
        frame = {
            "key": "0x52__EPAS3P_sysStatus",
            "address": 82,
            "addressHex": "0x52",
            "canonicalName": "EPAS3P_sysStatus",
            "module": "EPAS3P",
            "buses": ["CH"],
            "length": 8,
            "signalCount": 2,
        }
        layout_a = {
            "startBit": 19,
            "length": 12,
            "byteOrder": "big",
            "signed": False,
            "scale": 0.01,
            "offset": -20.5,
            "min": 0,
            "max": 0,
            "unit": "Nm",
            "mux": None,
        }
        layout_b = {
            "startBit": 39,
            "length": 2,
            "byteOrder": "big",
            "signed": False,
            "scale": 1,
            "offset": 0,
            "min": 0,
            "max": 3,
            "unit": "",
            "mux": None,
        }
        match_a = TCAN.Match(
            "EPAS3P_torsionBarTorque",
            frame,
            {"name": "EPAS3P_torsionBarTorque", "layout": layout_a, "presentIn": ["dbc:ModelY_CH"]},
            ["dbc:ModelY_CH"],
            ["dbc:ModelY_CH"],
            "https://example/frame",
        )
        match_b = TCAN.Match(
            "EPAS3P_handsOnLevel",
            frame,
            {"name": "EPAS3P_handsOnLevel", "layout": layout_b, "presentIn": ["dbc:ModelY_CH"]},
            ["dbc:ModelY_CH"],
            ["dbc:ModelY_CH"],
            "https://example/frame",
        )

        grouped = TCAN.grouped_matches_by_frame([match_a, match_b])
        html_text = TCAN.render_html(
            [match_a, match_b],
            [],
            "https://example",
            ["ModelY"],
            ["CH"],
            ["EPAS3P_torsionBarTorque", "EPAS3P_handsOnLevel"],
        )

        self.assertEqual(len(grouped), 1)
        self.assertEqual(len(grouped[0][1]), 2)
        self.assertIn('<div class="legend">', html_text)
        self.assertIn('Sticky byte index column', html_text)
        self.assertIn('Overlapping payload bit used by multiple selected signals', html_text)
        self.assertIn('<table class="bit-map overlap-map">', html_text)
        self.assertIn('position:sticky;top:8px', html_text)
        self.assertIn('<section id="0x52--EPAS3P-sysStatus" class="frame-card">', html_text)
        self.assertIn('<span class="sig">EPAS3P_torsionBarTorque</span>', html_text)
        self.assertIn('<span class="sig">EPAS3P_handsOnLevel</span>', html_text)


if __name__ == "__main__":
    unittest.main()
