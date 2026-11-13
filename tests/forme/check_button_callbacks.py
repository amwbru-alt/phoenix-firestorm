#!/usr/bin/env python3
"""Compile native button lambda signatures against the production callback contract."""
# Copyright (C) 2026 ForMe.World contributors. SPDX-License-Identifier: LGPL-2.1-or-later
import pathlib
import re
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]


def main():
    # Keep the actual LLUICtrl signal typedef and LLButton overload declaration.
    # A std::function-backed signal stub avoids needing the viewer's Boost package
    # before CI installs dependencies. It checks the same callable argument types.
    ctrl = (ROOT / 'indra/llui/lluictrl.h').read_text()
    button = (ROOT / 'indra/llui/llbutton.h').read_text()
    signal = re.search(r'typedef boost::signals2::signal<[^;]+> commit_signal_t;', ctrl).group()
    clicked = re.search(r'boost::signals2::connection setClickedCallback\(\s*const commit_signal_t::slot_type& cb\s*\);', button).group()
    prefix = '#include <functional>\nstruct LLSD {};\n'
    prefix += 'namespace boost { namespace signals2 { struct connection {}; template<class Signature> struct signal { using slot_type = std::function<Signature>; }; }}\n'
    prefix += 'struct LLUICtrl { ' + signal + ' };\nstruct LLButton : LLUICtrl { ' + clicked + ' };\n'
    declarations = []
    binding = re.compile(r'setClickedCallback\(\s*\[[^\]]*\]\s*\(([^()]*)\)')
    count = 0
    for path in sorted((ROOT / 'indra/newview').glob('*.cpp')):
        source = path.read_text()
        for match in binding.finditer(source):
            preceding = source[max(0, match.start() - 750):match.start()]
            line_prefix = preceding.rsplit('\n', 1)[-1]
            inline_button = re.search(r'(?:getChild|findChild)\s*<\s*LLButton\s*>', line_prefix)
            receiver = re.search(r'(\w+)->\s*$', preceding)
            local_button = receiver and re.search(
                r'\b' + re.escape(receiver[1]) + r'\s*=\s*(?:findChild|getChild|LLUICtrlFactory::create)\s*<\s*LLButton\s*>', preceding)
            if not inline_button and not local_button:
                continue
            count += 1
            relative = path.relative_to(ROOT).as_posix()
            line = source.count('\n', 0, match.start()) + 1
            # Only the lambda signature participates in slot conversion; capture
            # names and the GUI body are irrelevant to this compiler diagnostic.
            declarations.append(f'#line {line} "{relative}"\nvoid probe_{count}(LLButton& b) {{ b.setClickedCallback([]({match[1]}) {{}}); }}\n')
    if not count:
        raise RuntimeError('No native LLButton lambda bindings found')
    with tempfile.TemporaryDirectory() as directory:
        probe = pathlib.Path(directory) / 'button_callbacks.cpp'
        probe.write_text(prefix + '\n'.join(declarations))
        result = subprocess.run(['c++', '-std=c++17', '-fsyntax-only', str(probe)], capture_output=True, text=True)
        if result.returncode:
            print('Native LLButton callback signature mismatch:\n' + result.stderr, file=sys.stderr)
            return 1
    print(f'PASS {count} native button lambda signatures against LLButton/LLUICtrl declarations')
    return 0


if __name__ == '__main__':
    sys.exit(main())
