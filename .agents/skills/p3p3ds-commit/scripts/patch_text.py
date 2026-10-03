"""Exact text replacement that keeps each file's line endings.

Several files mix CRLF (upstream) and LF (P3P3DS patches) lines, e.g.
recomp/PSPRecomp/src/runtime.cpp. Text-mode Python, sed -i and editors can
rewrite every line ending; this matches a block whatever its line endings
are and writes the replacement with the endings found there.

usage as a module (from a scratch script in .tmp/):
    import sys; sys.path.insert(0, '.claude/skills/p3p3ds-commit/scripts')
    from patch_text import patch
    patch('path/to/file.cpp', [(old_block, new_block), ...])
Each old block must occur exactly once (AssertionError otherwise).
Write scratch scripts with the editor's Write tool: shell heredocs on this
host turn '\\n' inside Python strings into real newlines.
"""
import re


def patch(path, pairs):
    text = open(path, 'rb').read().decode('utf-8')
    for old, new in pairs:
        pattern = r'\r?\n'.join(re.escape(line) for line in old.split('\n'))
        matches = list(re.finditer(pattern, text))
        assert len(matches) == 1, (path, len(matches), old[:80])
        m = matches[0]
        found = m.group(0)
        eol = '\r\n' if '\r\n' in found else '\n'
        if '\n' not in found:  # single line: use the ending of the line it sits on
            k = text.find('\n', m.end())
            eol = '\r\n' if k > 0 and text[k - 1] == '\r' else '\n'
        text = text[:m.start()] + new.replace('\n', eol) + text[m.end():]
    open(path, 'wb').write(text.encode('utf-8'))
