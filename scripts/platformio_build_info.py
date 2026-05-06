from datetime import datetime, timezone
import hashlib
from pathlib import Path
import re
import subprocess

from SCons.Script import Import

Import("env")

ROOT = Path(env["PROJECT_DIR"]).resolve()


def run_git(args, fallback="unknown"):
    try:
        return subprocess.check_output(
            ["git", *args], cwd=ROOT, text=True, stderr=subprocess.DEVNULL
        ).strip() or fallback
    except Exception:
        return fallback


def read_version():
    text = (ROOT / "include" / "version.h").read_text(errors="ignore")
    match = re.search(r'#define\s+FIRMWARE_VERSION\s+"([^"]+)"', text)
    return match.group(1) if match else "0.0.0"


def source_hash():
    hasher = hashlib.sha1()
    roots = [ROOT / "include", ROOT / "src", ROOT / "scripts"]
    files = [ROOT / "platformio.ini", ROOT / "VERSION"]
    for root in roots:
        if root.exists():
            files.extend(
                p for p in root.rglob("*")
                if p.is_file() and "__pycache__" not in p.parts
            )
    for path in sorted(files):
        if not path.exists():
            continue
        rel = path.relative_to(ROOT).as_posix().encode()
        hasher.update(rel + b"\0")
        hasher.update(path.read_bytes() + b"\0")
    return hasher.hexdigest()[:12]


def stringify(value):
    escaped = str(value).replace("\\", "\\\\").replace('"', '\\"')
    return f'\\"{escaped}\\"'


version = read_version()
compact_version = version.replace(".", "")
now = datetime.now(timezone.utc).astimezone()
build_stamp = now.strftime("%y%m%d%H%M")
build_at = now.isoformat(timespec="seconds")
build_env = env.subst("$PIOENV") or "unknown"
safe_build_env = re.sub(r"[^A-Za-z0-9]+", "", build_env)[:24] or "env"
git_sha = run_git(["rev-parse", "--short=7", "HEAD"])
git_branch = run_git(["rev-parse", "--abbrev-ref", "HEAD"])
dirty_status = run_git(
    ["status", "--short", "--untracked-files=all", "--", "include", "src", "scripts", "platformio.ini", "VERSION"],
    fallback="",
)
git_dirty = 1 if dirty_status else 0
src_hash = source_hash()
build_id = f"FW{compact_version}-{build_stamp}-{safe_build_env}-{git_sha}-{src_hash}{'D' if git_dirty else 'C'}"

env.Append(
    CPPDEFINES=[
        ("FIRMWARE_BUILD_ID", stringify(build_id)),
        ("FIRMWARE_BUILD_ENV", stringify(build_env)),
        ("FIRMWARE_BUILD_AT", stringify(build_at)),
        ("FIRMWARE_GIT_SHA", stringify(git_sha)),
        ("FIRMWARE_GIT_BRANCH", stringify(git_branch)),
        ("FIRMWARE_SOURCE_HASH", stringify(src_hash)),
        ("FIRMWARE_GIT_DIRTY", git_dirty),
    ]
)

print(f"Build info: {build_id} ({build_env}, {git_branch}@{git_sha}, dirty={git_dirty})")