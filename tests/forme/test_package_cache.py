#!/usr/bin/env python3
"""Regression checks for verified autobuild cache downloads.

$LicenseInfo:firstyear=2026&license=viewerlgpl$
Copyright (C) 2026 ForMe.World contributors.
SPDX-License-Identifier: LGPL-2.1-or-later
$/LicenseInfo$
"""
import hashlib
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('package_cache', ROOT / 'scripts/forme_cache_package.py')
cache_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cache_module)


class PackageCacheTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.payload = b'complete package bytes'
        self.manifest = self.root / 'autobuild.xml'
        self.manifest.write_text('''<llsd><map><key>installables</key><map>
          <key>webrtc</key><map><key>platforms</key><map><key>darwin64</key><map>
          <key>archive</key><map><key>url</key><string>https://example.test/archive.tar.zst</string>
          <key>hash</key><string>''' + hashlib.sha1(self.payload).hexdigest() + '''</string>
          <key>hash_algorithm</key><string>sha1</string></map></map></map></map></map></map></llsd>''')
        self.cache = self.root / 'cache'
        self.target = self.cache / 'archive.tar.zst'

    def run_cache(self):
        return cache_module.cache_package(self.manifest, 'webrtc', 'darwin64', self.cache)

    def download(self, command, **kwargs):
        Path(command[command.index('--output') + 1]).write_bytes(self.payload)

    def test_verified_download_is_reused_without_network(self):
        with patch.object(cache_module.subprocess, 'run', side_effect=self.download) as download:
            self.assertEqual(self.target, self.run_cache())
            self.assertEqual(self.target, self.run_cache())
        self.assertEqual(1, download.call_count)
        self.assertEqual(self.payload, self.target.read_bytes())
        self.assertEqual([self.target], list(self.cache.iterdir()))

    def test_corrupted_cache_is_replaced_only_after_verification(self):
        self.cache.mkdir()
        self.target.write_bytes(b'truncated cached package')
        with patch.object(cache_module.subprocess, 'run', side_effect=self.download):
            self.run_cache()
        self.assertEqual(self.payload, self.target.read_bytes())

    def test_bad_checksum_does_not_publish_download(self):
        def bad_download(command, **kwargs):
            Path(command[command.index('--output') + 1]).write_bytes(b'wrong archive')
        with patch.object(cache_module.subprocess, 'run', side_effect=bad_download):
            with self.assertRaisesRegex(ValueError, 'Checksum mismatch'):
                self.run_cache()
        self.assertEqual([], list(self.cache.iterdir()))

    def test_network_failure_does_not_publish_partial_download(self):
        def failed_download(command, **kwargs):
            self.download(command)
            raise subprocess.CalledProcessError(22, command)
        with patch.object(cache_module.subprocess, 'run', side_effect=failed_download):
            with self.assertRaises(subprocess.CalledProcessError):
                self.run_cache()
        self.assertEqual([], list(self.cache.iterdir()))


if __name__ == '__main__':
    unittest.main()
