#!/usr/bin/env python3
"""Warm autobuild's cache with a verified public package and paced HTTP retries.

$LicenseInfo:firstyear=2026&license=viewerlgpl$
Copyright (C) 2026 ForMe.World contributors.
SPDX-License-Identifier: LGPL-2.1-or-later
$/LicenseInfo$
"""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
from urllib.parse import urlsplit
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]


def entry(node, key):
    children = list(node)
    for index in range(0, len(children), 2):
        if children[index].text == key:
            return children[index + 1]
    raise ValueError(f'Missing manifest key: {key}')


def digest(path, algorithm):
    result = hashlib.new(algorithm)
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            result.update(block)
    return result.hexdigest()


def cache_package(manifest, package, platform, cache):
    root = ET.parse(manifest).getroot().find('map')
    archive = root
    for key in ('installables', package, 'platforms', platform, 'archive'):
        archive = entry(archive, key)
    # Authenticated packages must continue to use autobuild's credential handling.
    if any(key.text == 'creds' for key in archive.findall('key')):
        raise ValueError('Only public package archives can be cached by this helper')
    url = entry(archive, 'url').text
    expected = entry(archive, 'hash').text.lower()
    algorithm = entry(archive, 'hash_algorithm').text
    filename = Path(urlsplit(url).path).name
    if not filename:
        raise ValueError('Archive URL has no filename')
    cache.mkdir(parents=True, exist_ok=True)
    target = cache / filename
    if target.is_file() and digest(target, algorithm) == expected:
        print(f'Using verified cached {package}')
        return target
    # Never expose a partial download to autobuild, including on checksum failure.
    with tempfile.TemporaryDirectory(prefix='download-', dir=cache) as temp:
        download = Path(temp) / filename
        subprocess.run([
            'curl', '--fail', '--location', '--show-error', '--silent',
            '--retry', '5', '--retry-all-errors', '--retry-delay', '10',
            '--connect-timeout', '30', '--max-time', '900',
            '--output', str(download), url
        ], check=True)
        if digest(download, algorithm) != expected:
            raise ValueError(f'Checksum mismatch for {package}; archive rejected')
        download.replace(target)
    print(f'Cached and verified {package}')
    return target


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('package')
    parser.add_argument('platform')
    args = parser.parse_args()
    cache = os.environ.get('AUTOBUILD_INSTALLABLE_CACHE')
    if not cache:
        parser.error('AUTOBUILD_INSTALLABLE_CACHE must be set')
    cache_package(ROOT / 'autobuild.xml', args.package, args.platform, Path(cache))


if __name__ == '__main__':
    main()
