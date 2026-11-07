#!/usr/bin/env python3
# Copyright (C) 2026, ForMe.World contributors.
# SPDX-License-Identifier: LGPL-2.1-only
"""Recognize the existing Firestorm, Linden and standard third-party notices."""
from pathlib import Path
import re
import sys


def notice_errors(text):
    header = text[:8192]
    flat = re.sub(r'[\s*#]+', ' ', header)
    project_tag = re.search(r'\$LicenseInfo:[^$]*\blicense=(?:fsviewerlgpl|viewerlgpl|lgpl|bsd|mit)\b', header, re.I)
    spdx_match = re.search(r'SPDX-License-Identifier:\s*([A-Za-z0-9.+-]+)', header)
    spdx = spdx_match and spdx_match.group(1) in {
        'LGPL-2.1-only', 'LGPL-2.1-or-later', 'LGPL-3.0-only', 'LGPL-3.0-or-later',
        'GPL-2.0-only', 'GPL-2.0-or-later', 'GPL-3.0-only', 'GPL-3.0-or-later',
        'MIT', 'BSD-2-Clause', 'BSD-3-Clause', 'Apache-2.0'}
    gnu = re.search(r'GNU (?:Lesser )?General Public License', flat)
    bsd = 'Redistribution and use in source and binary forms' in flat and 'THIS SOFTWARE IS PROVIDED' in flat
    mit = 'Permission is hereby granted, free of charge' in flat and 'THE SOFTWARE IS PROVIDED' in flat
    errors = []
    if not (project_tag or spdx or gnu or bsd or mit):
        errors.append('Missing recognized opensource license notice')
    if not re.search(r'\bcopyright\b', header, re.I):
        errors.append('Missing copyright notice')
    return errors


def main(paths):
    failed = False
    for name in paths:
        try:
            errors = notice_errors(Path(name).read_text(encoding='utf-8'))
        except UnicodeError:
            errors = ['Source is not valid UTF-8']
        for error in errors:
            print(f'{name}: {error}', file=sys.stderr)
            failed = True
    return int(failed)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
