#!/usr/bin/env python3
"""Verify that committed media matches its manifest and captured source."""

import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEMO = ROOT / "demo"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


source = json.loads((DEMO / "source-manifest.json").read_text())
for relative, expected in source["files"].items():
    actual = sha256(ROOT / relative)
    if actual != expected:
        raise SystemExit(f"source drift: {relative}: {actual} != {expected}")

capture = json.loads((DEMO / "capture.json").read_text())
for relative, metadata in capture["artifacts"].items():
    path = ROOT / relative
    if path.stat().st_size != metadata["bytes"]:
        raise SystemExit(f"size mismatch: {relative}")
    actual = sha256(path)
    if actual != metadata["sha256"]:
        raise SystemExit(f"sha256 mismatch: {relative}: {actual}")

actual_output = (DEMO / "actual-output.txt").read_text()
for evidence in capture["required_output"]:
    if evidence not in actual_output:
        raise SystemExit(f"captured output missing evidence: {evidence}")
print("Demo source, output, and artifact hashes verified.")
