#!/usr/bin/env python3
"""Fast source checks; these do not replace a complete viewer build.

$LicenseInfo:firstyear=2026&license=viewerlgpl$
Copyright (C) 2026 ForMe.World contributors.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
$/LicenseInfo$
"""
import pathlib
import re
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

ROOT = pathlib.Path(__file__).resolve().parents[1]


def main():
    errors = []
    tracked = set(subprocess.check_output(
        ['git', 'ls-files'], cwd=ROOT, text=True).splitlines())
    # Check literal entries in source/header lists against the complete Git tree,
    # including in sparse working copies. Generated variable paths are skipped.
    for rel in ('indra/cmake/CMakeLists.txt', 'indra/newview/CMakeLists.txt'):
        path = ROOT / rel
        for block in re.finditer(
                r'(?:set|list)\s*\((?:APPEND\s+)?\w*(?:SOURCE_FILES|HEADER_FILES)\b([^)]*)\)',
                path.read_text(), re.S):
            body = re.sub(r'#[^\n]*', '', block.group(1))
            for token in body.split():
                token = token.strip('"')
                if '$' in token or not re.search(r'\.(?:cpp|h|cmake)$', token):
                    continue
                target = (path.parent.relative_to(ROOT) / token).as_posix()
                if target not in tracked:
                    errors.append(f'{rel}: missing source {token}')

    literal = re.compile(
        r'getChild\s*<\s*LLTextBox\s*>\s*\([^;]*?\)\s*->\s*setText\s*\(\s*"', re.S)
    for path in (ROOT / 'indra/newview').glob('*.cpp'):
        source = path.read_text()
        for match in literal.finditer(source):
            line = source.count('\n', 0, match.start()) + 1
            errors.append(f'{path.name}:{line}: use std::string or LLStringExplicit for LLTextBox::setText')

    # Do not reintroduce Chromium to repair compilation after browser removal.
    for rel in ('indra/CMakeLists.txt', 'indra/media_plugins/CMakeLists.txt',
                'indra/cmake/CMakeLists.txt', 'indra/newview/CMakeLists.txt'):
        source = re.sub(r'#[^\n]*', '', (ROOT / rel).read_text())
        if re.search(r'CEFPlugin|media_plugins/cef|add_subdirectory\s*\(\s*cef\b', source, re.I):
            errors.append(f'{rel}: deleted CEF build dependency')
    packages = ET.parse(ROOT / 'autobuild.xml')
    if any((key.text or '').lower() in ('dullahan', 'cef', 'cefplugin')
           for key in packages.iter('key')):
        errors.append('autobuild.xml: browser package dependency')
    manifest = (ROOT / 'indra/newview/viewer_manifest.py').read_text()
    compile(manifest, 'viewer_manifest.py', 'exec')
    active_manifest = '\n'.join(line for line in manifest.splitlines()
                                if not line.lstrip().startswith('#'))
    if re.search(r'media_plugin_cef|Chromium Embedded Framework|dullahan_host', active_manifest, re.I):
        errors.append('viewer_manifest.py: browser runtime packaging')
    if 'LLViewerMedia::' in (ROOT / 'indra/newview/llweb.cpp').read_text():
        errors.append('llweb.cpp: obsolete embedded-browser media callback')

    # Compile a regression probe using the actual string class and actual
    # LLTextBox declaration. Verify the bad call fails and the replacement works.
    header = (ROOT / 'indra/llcommon/llstring.h').read_text()
    string_type = re.search(r'class LLStringExplicit\s*:[\s\S]*?\n};', header).group()
    declaration = re.search(r'void setText\([^;]+;',
                            (ROOT / 'indra/llui/lltextbox.h').read_text()).group()
    declaration = declaration.replace(' override;', ';')
    prefix = '#include <string>\n' + string_type + '\n'
    prefix += 'struct LLStyle { struct Params {}; };\n'
    prefix += 'struct LLTextBox { ' + declaration + ' };\n'
    with tempfile.TemporaryDirectory() as temp:
        probe = pathlib.Path(temp) / 'textbox.cpp'
        for argument, expected in [('""', False), ('std::string()', True)]:
            probe.write_text(prefix + '\nvoid probe(LLTextBox& box) { box.setText(' + argument + '); }\n')
            result = subprocess.run(['c++', '-std=c++17', '-fsyntax-only', str(probe)],
                                    capture_output=True, text=True)
            if (result.returncode == 0) != expected:
                errors.append('LLTextBox type regression probe failed: ' + result.stderr)

    if errors:
        print('\n'.join(errors), file=sys.stderr)
        return 1
    print('ForMe preflight passed: source lists, text types, browser dependencies and packaging')
    return 0


if __name__ == '__main__':
    sys.exit(main())
