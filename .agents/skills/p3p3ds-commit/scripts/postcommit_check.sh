#!/usr/bin/env bash
# Post-commit proof (CLAUDE.md section 17): HEAD^ must equal the HEAD recorded
# before the task, and the message must carry no AI attribution trailer.
#   postcommit_check.sh <head_before>
set -uo pipefail
before=${1:?HEAD recorded before the task}
git rev-parse HEAD
git rev-parse HEAD^
git log -2 --oneline
if [ "$(git rev-parse HEAD^)" != "$(git rev-parse "$before")" ]; then echo "FAIL: parent is not $before"; exit 1; fi
if git log -1 --format=%B | grep -Eiq '^[[:space:]]*(Co-Authored-By|Generated-By|Assisted-By):'; then echo "FAIL: attribution trailer in message"; exit 1; fi
echo "POSTCOMMIT OK"
