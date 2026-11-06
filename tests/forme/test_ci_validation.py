#!/usr/bin/env python3
# Copyright (C) 2026, ForMe.World contributors.
# SPDX-License-Identifier: LGPL-2.1-only
"""Prevent false license failures without accepting missing source notices."""
import importlib.util
import contextlib
import io
import subprocess
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('license_check', ROOT / 'scripts/forme_check_license.py')
check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check)


class LicenseNotices(unittest.TestCase):
    def test_existing_project_notices(self):
        for tag in ('viewerlgpl', 'fsviewerlgpl', 'bsd', 'mit'):
            self.assertEqual([], check.notice_errors(f'/* $LicenseInfo:firstyear=2026&license={tag}$ Copyright (c) Original Author */'))

    def test_standard_notices(self):
        for notice in ('SPDX-License-Identifier: LGPL-2.1-only',
                       'GNU\n * Lesser General Public License',
                       'Redistribution and use in source and binary forms\n * THIS SOFTWARE IS PROVIDED'):
            self.assertEqual([], check.notice_errors('Copyright (c) Original Author\n' + notice))

    def test_missing_notices_fail(self):
        self.assertEqual(2, len(check.notice_errors('int main() { return 0; }')))
        self.assertEqual(['Missing copyright notice'], check.notice_errors('SPDX-License-Identifier: MIT'))
        self.assertEqual(['Missing recognized opensource license notice'], check.notice_errors('Copyright (c) Original Author'))

    def test_unknown_spdx_fails(self):
        self.assertTrue(check.notice_errors('Copyright (c) Author\nSPDX-License-Identifier: MIT-unknown'))

    def test_invalid_encoding_fails(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'source.cpp'
            path.write_bytes(b'\x93invalid')
            output = io.StringIO()
            with contextlib.redirect_stderr(output):
                self.assertEqual(1, check.main([str(path)]))
            self.assertIn('not valid UTF-8', output.getvalue())


class PullRequestRange(unittest.TestCase):
    def test_shallow_merge_includes_all_feature_changes(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp) / 'source'
            repo.mkdir()
            def git(*args, cwd=repo):
                return subprocess.run(['git', *args], cwd=cwd, check=True, capture_output=True, text=True).stdout.strip()
            git('init', '-b', 'main')
            git('config', 'user.name', 'CI fixture')
            git('config', 'user.email', 'ci@example.invalid')
            (repo / 'legacy.cpp').write_text('legacy baseline with no notice\n')
            git('add', '.')
            git('commit', '-m', 'baseline')
            base = git('rev-parse', 'HEAD')
            git('checkout', '-b', 'feature')
            for filename in ('first.cpp', 'second.cpp'):
                (repo / filename).write_text('changed source\n')
                git('add', filename)
                git('commit', '-m', filename)
            git('checkout', 'main')
            git('merge', '--no-ff', 'feature', '-m', 'PR merge')
            clone = Path(temp) / 'checkout'
            git('clone', '--depth=2', repo.as_uri(), str(clone))
            changed = git('diff', '--name-only', base + '...HEAD', cwd=clone).splitlines()
            self.assertEqual(['first.cpp', 'second.cpp'], changed)


if __name__ == '__main__':
    unittest.main()
