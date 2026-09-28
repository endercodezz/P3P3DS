#!/usr/bin/env python3
"""Configure-time staleness check for output/fingerprint.json (issue #36).

Recomputes the emitter-source content hash and compares it with the one
recorded by `psprecomp recompile`. The hash recipe (version 1) MUST stay in
sync with crates/psp-cli/src/fingerprint.rs:

  1. File set: every *.rs file (recursive) under each of hash_recipe.roots,
     plus each of hash_recipe.extra_files; paths repo-root-relative, "/"
     separators.
  2. Per-file hash: SHA-256 of raw bytes, lowercase hex.
  3. Combined hash: SHA-256 over the concatenation of "{path}\n{hash}\n" for
     every file, sorted by path.

List divergence is impossible by construction: per-file comparison runs over
exactly the files recorded in fingerprint.json. The roots are re-enumerated
(from the JSON, not a duplicated list) only to detect files ADDED after the
recompile, which the recorded list cannot cover.

Exit codes (consumed by runtime/CMakeLists.txt):
  0  fingerprint matches
  2  STALE: sources changed since recompile (details on stdout)
  3  fingerprint.json missing (pre-#36 output dir)
  4  unverifiable (sources unavailable at recompile time, bad JSON, or a
     newer hash_recipe version)
"""

import argparse
import hashlib
import json
import sys
from pathlib import Path

HASH_RECIPE_VERSION = 1


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def combined_hash(entries):
    """entries: iterable of (path, sha256_hex); recipe step 3."""
    h = hashlib.sha256()
    for path, digest in sorted(entries):
        h.update(path.encode())
        h.update(b"\n")
        h.update(digest.encode())
        h.update(b"\n")
    return h.hexdigest()


def enumerate_file_set(repo_root: Path, roots, extra_files):
    """Recipe step 1, re-derived from the JSON's own hash_recipe."""
    rel_paths = set()
    for root in roots:
        for p in (repo_root / root).rglob("*.rs"):
            if p.is_file():
                rel_paths.add(p.relative_to(repo_root).as_posix())
    for extra in extra_files:
        rel_paths.add(extra)
    return rel_paths


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--fingerprint", required=True, type=Path)
    ap.add_argument("--repo-root", required=True, type=Path)
    args = ap.parse_args()
    repo_root = args.repo_root.resolve()

    if not args.fingerprint.is_file():
        print(f"fingerprint.json not found: {args.fingerprint}")
        return 3

    try:
        fp = json.loads(args.fingerprint.read_text())
    except (OSError, ValueError) as e:
        print(f"cannot parse {args.fingerprint}: {e}")
        return 4

    recorded = fp.get("emitter_sources_hash", "")
    recipe = fp.get("hash_recipe", {})
    files = fp.get("files", [])
    if recorded in ("", "unavailable") or not files:
        print("fingerprint records no source hashes (emitter sources were "
              "unavailable at recompile time) -- staleness cannot be verified")
        return 4
    if recipe.get("version") != HASH_RECIPE_VERSION:
        print(f"hash_recipe version {recipe.get('version')} != supported "
              f"{HASH_RECIPE_VERSION} -- update check_fingerprint.py")
        return 4

    # Per-file comparison over exactly the recorded list.
    changed, missing = [], []
    actual_entries = []
    for entry in files:
        rel, want = entry["path"], entry["sha256"]
        path = repo_root / rel
        if not path.is_file():
            missing.append(rel)
            continue
        got = sha256_file(path)
        actual_entries.append((rel, got))
        if got != want:
            changed.append(rel)

    # Files added since the recompile (re-enumerate roots from the JSON).
    listed = {entry["path"] for entry in files}
    on_disk = enumerate_file_set(
        repo_root, recipe.get("roots", []), recipe.get("extra_files", []))
    added = sorted(on_disk - listed)

    actual = combined_hash(
        actual_entries + [(rel, sha256_file(repo_root / rel)) for rel in added])

    if not changed and not missing and not added:
        ts = fp.get("generated_at", "?")
        cross_mid = 1 if fp.get("flags", {}).get("cross_mid") else 0
        print(f"output/ fingerprint OK: {recorded} "
              f"(cross_mid={cross_mid}, recompiled {ts})")
        # Belt-and-braces: identical per-file hashes must combine identically.
        if actual != recorded:
            print("INTERNAL: combined hash mismatch despite identical files -- "
                  "recipe divergence between fingerprint.rs and this script")
            return 2
        # analysis.json drift is stale too (re-analyze without recompile).
        rec_analysis = fp.get("analysis", {})
        apath = Path(rec_analysis.get("path", ""))
        if not apath.is_absolute():
            apath = repo_root / apath
        if rec_analysis.get("sha256") and apath.is_file():
            if sha256_file(apath) != rec_analysis["sha256"]:
                print(f"STALE: {apath} changed since output/ was recompiled")
                return 2
        return 0

    print(f"STALE: emitter sources changed since output/ was recompiled "
          f"(recorded {recorded[:16]}..., actual {actual[:16]}...)")
    for rel in changed:
        print(f"  changed: {rel}")
    for rel in missing:
        print(f"  deleted: {rel}")
    for rel in added:
        print(f"  added:   {rel}")
    return 2


if __name__ == "__main__":
    sys.exit(main())
