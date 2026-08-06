"""lilygo_t2can 성공 빌드 후 OTA/실차 테스트용 bin 복사본을 만든다."""

from datetime import datetime
from pathlib import Path
import re
import shutil
from zoneinfo import ZoneInfo

from SCons.Script import Import

Import("env")

ROOT = Path(env["PROJECT_DIR"]).resolve()
VERSION_HEADER = ROOT / "include" / "version.h"


def header_define(name: str, fallback: str) -> str:
    text = VERSION_HEADER.read_text(encoding="utf-8", errors="ignore")
    match = re.search(rf'#define\s+{re.escape(name)}\s+"([^"]+)"', text)
    return match.group(1) if match else fallback


def safe_name_part(value: str, fallback: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9._-]+", "-", value).strip(".-")
    return cleaned or fallback


def copy_artifact(source, target, env):
    source_path = Path(str(source[0]))
    if not source_path.is_file():
        print(f"Firmware artifact skipped: missing {source_path}")
        return
    version = safe_name_part(header_define("FIRMWARE_VERSION", "0.0.0"), "0.0.0")
    note = safe_name_part(header_define("FIRMWARE_ARTIFACT_NOTE", "firmware"), "firmware")
    kst_date = datetime.now(ZoneInfo("Asia/Seoul")).strftime("%y-%m-%d")
    artifact = ROOT / f"{version}_{kst_date}_{note}.bin"
    shutil.copy2(source_path, artifact)
    print(f"Firmware artifact: {artifact.name}")


if env.subst("$PIOENV") == "lilygo_t2can":
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_artifact)
    print("Firmware artifact hook: enabled")
