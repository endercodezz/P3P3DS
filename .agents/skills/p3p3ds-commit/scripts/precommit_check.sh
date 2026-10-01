#!/usr/bin/env bash
# Pre-commit gate for P3P3DS (CLAUDE.md sections 15-17). Run from the
# repository root after `git add`. Prints HEAD (record it), staged files,
# whitespace errors, forbidden staged paths and line-ending churn.
set -uo pipefail
status=0
echo "HEAD before: $(git rev-parse HEAD)"
echo "--- staged"
git diff --cached --stat
if ! git diff --cached --check; then echo "FAIL: whitespace errors"; status=1; fi
forbidden='^(\.tmp/|\.cache/|build/|out/|\.claude/LOCAL\.md|\.claude/settings\.local\.json)|\.(iso|cso|pbp|elf|prx|cpk|pmsf|pmf|at3|bin|wav|bmp)$'
bad=$(git diff --cached --name-only | grep -Ei "$forbidden" | grep -v '^references/' || true)
if [ -n "$bad" ]; then echo "FAIL: forbidden staged paths:"; echo "$bad"; status=1; fi
# Line-ending churn: a file whose diff ignoring CR differs much less than the raw diff.
for f in $(git diff --cached --name-only --diff-filter=M); do
    raw=$(git diff --cached --numstat -- "$f" | awk '{print $1+$2}')
    nocr=$(git diff --cached --numstat --ignore-cr-at-eol -- "$f" | awk '{print $1+$2}')
    if [ -n "$raw" ] && [ -n "$nocr" ] && [ "$raw" -gt $((nocr + 4)) ]; then
        echo "WARN: $f has EOL churn ($raw changed lines, $nocr ignoring CR) - re-apply the edit byte-exactly"
        status=1
    fi
done
if git diff --cached | grep -Eiq '^\+[[:space:]]*(Co-Authored-By|Generated-By|Assisted-By):'; then
    echo "FAIL: AI attribution text in staged diff"; status=1
fi
# Tracked files are public: no machine-specific absolute paths (drive letters, user home dirs).
paths=$(git diff --cached -U0 | grep -E '^\+' | grep -v '^+++' | grep -En '[A-Za-z]:\\[A-Za-z0-9_]|/[a-z]/Users/|[A-Za-z]:/Users/' || true)
if [ -n "$paths" ]; then echo "FAIL: absolute local paths in staged lines:"; echo "$paths" | head -5; status=1; fi
[ $status -eq 0 ] && echo "PRECOMMIT OK"
exit $status
