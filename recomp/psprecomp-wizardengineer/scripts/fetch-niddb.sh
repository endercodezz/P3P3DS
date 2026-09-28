#!/usr/bin/env bash
# fetch-niddb.sh — bootstrap the PSP NID database required by `psprecomp analyze`.
#
# WHAT THIS FETCHES
#   data/niddb/ppsspp_niddb.xml — a flat <NID> -> <NAME> map of ~2183 PSP
#   firmware function NIDs (the 4-byte SHA-1-derived identifiers Sony's PRX
#   format uses for imports/exports) to their human-readable names. `analyze`
#   needs it to resolve the binary's import stubs to real SDK function names
#   (e.g. NID 0x... -> sceGeListEnQueue) instead of NID_0xXXXXXXXX fallbacks.
#
# WHY A SCRIPT (not a committed file)
#   The NID names are derived from PPSSPP (GPL-2.0-or-later). This repo is also
#   GPL-2.0-or-later and already attributes PPSSPP, so vendoring would be
#   license-compatible — but the canonical, maintained copy lives upstream in
#   pspdev/psp-ghidra-scripts, so we fetch it from there to stay in sync and
#   avoid carrying a stale duplicate. `data/` is gitignored.
#
# SOURCE / ATTRIBUTION
#   File:    ppsspp_niddb.xml
#   Repo:    https://github.com/pspdev/psp-ghidra-scripts  (NID list derived
#            from PPSSPP source — https://github.com/hrydgard/ppsspp)
#   License: PPSSPP is GPL-2.0-or-later; the NID->name mappings are factual.
#
# IDEMPOTENT: re-running with the file already present and matching the pinned
# checksum is a no-op. Exits non-zero with a clear message if the source is
# unreachable or the download fails verification.

set -euo pipefail

# Resolve repo root from this script's location (scripts/ -> repo root).
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

DEST_DIR="${REPO_ROOT}/data/niddb"
DEST="${DEST_DIR}/ppsspp_niddb.xml"

# Pinned upstream source. The raw file on the repo's default branch.
URL="https://raw.githubusercontent.com/pspdev/psp-ghidra-scripts/master/ppsspp_niddb.xml"

# Known-good checksum of the working database (the copy this project analyzes
# Patapon BOOT.BIN with). The fetched file is verified against this so a
# changed/truncated upstream can never silently produce a different DB.
EXPECTED_SHA256="7758400d1c176f94401bbfa8bca33a37b209b998d7ef10e85fe606083f2dd55f"

sha256_of() {
    if command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | awk '{print $1}'
    elif command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    else
        echo "ERROR: neither shasum nor sha256sum found; cannot verify download." >&2
        exit 1
    fi
}

# Idempotent: already present and correct -> done.
if [[ -f "${DEST}" ]] && [[ "$(sha256_of "${DEST}")" == "${EXPECTED_SHA256}" ]]; then
    echo "NID DB already present and verified: ${DEST}"
    echo "Source: ${URL} (PPSSPP-derived, GPL-2.0-or-later)"
    exit 0
fi

mkdir -p "${DEST_DIR}"
TMP="$(mktemp "${DEST_DIR}/.niddb.XXXXXX")"
trap 'rm -f "${TMP}"' EXIT

echo "Fetching NID DB from ${URL}"
if command -v curl >/dev/null 2>&1; then
    if ! curl -fsSL "${URL}" -o "${TMP}"; then
        echo "ERROR: download failed (curl). Source unreachable: ${URL}" >&2
        echo "Check your network, or fetch the file manually into ${DEST}." >&2
        exit 1
    fi
elif command -v wget >/dev/null 2>&1; then
    if ! wget -q "${URL}" -O "${TMP}"; then
        echo "ERROR: download failed (wget). Source unreachable: ${URL}" >&2
        echo "Check your network, or fetch the file manually into ${DEST}." >&2
        exit 1
    fi
else
    echo "ERROR: neither curl nor wget found; cannot download." >&2
    exit 1
fi

GOT_SHA="$(sha256_of "${TMP}")"
if [[ "${GOT_SHA}" != "${EXPECTED_SHA256}" ]]; then
    echo "ERROR: downloaded NID DB checksum mismatch." >&2
    echo "  expected: ${EXPECTED_SHA256}" >&2
    echo "  got:      ${GOT_SHA}" >&2
    echo "Upstream may have changed. Inspect the file, and if the change is" >&2
    echo "legitimate, update EXPECTED_SHA256 in $(basename "${BASH_SOURCE[0]}")." >&2
    exit 1
fi

mv "${TMP}" "${DEST}"
trap - EXIT
echo "Wrote ${DEST}"
echo "  $(grep -c '<FUNCTION>' "${DEST}") function NID entries, sha256 ${GOT_SHA}"
echo "Source: ${URL} (PPSSPP-derived NID names, GPL-2.0-or-later)"
