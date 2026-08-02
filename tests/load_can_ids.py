"""Load #define values from ../can_ids.h so C and tests share one source."""

from __future__ import annotations

import re
from pathlib import Path

_HEADER = Path(__file__).resolve().parents[1] / "can_ids.h"
_DEFINE_RE = re.compile(
    r"^\s*#define\s+(CAN_\w+)\s+(0x[0-9A-Fa-f]+u?|\d+u?)\s*$"
)


def load_can_ids(header: Path = _HEADER) -> dict[str, int]:
    ids: dict[str, int] = {}
    text = header.read_text(encoding="utf-8")
    for line in text.splitlines():
        match = _DEFINE_RE.match(line)
        if not match:
            continue
        name, raw = match.group(1), match.group(2).rstrip("uU")
        ids[name] = int(raw, 0)
    if "CAN_STEERING_THROTTLE" not in ids:
        raise RuntimeError(f"failed to parse CAN IDs from {header}")
    return ids


CAN = load_can_ids()
