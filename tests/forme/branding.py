#  * @file
#  * @brief
#  *
#  * $LicenseInfo:firstyear=2026&license=viewerlgpl$
#  * Phoenix Firestorm Viewer Source Code
#  * Copyright (C) 2026, ForMe.World contributors.
#  *
#  * This library is free software; you can redistribute it and/or
#  * modify it under the terms of the GNU Lesser General Public
#  * License as published by the Free Software Foundation;
#  * version 2.1 of the License only.
#  *
#  * This library is distributed in the hope that it will be useful,
#  * but WITHOUT ANY WARRANTY; without even the implied warranty of
#  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  * Lesser General Public License for more details.
#  *
#  * You should have received a copy of the GNU Lesser General Public
#  * License along with this library; if not, write to the Free Software
#  * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#  *
#  * The Phoenix Firestorm Project, Inc., 1831 Oakwood Drive, Fairmont, Minnesota 56031-3225 USA
#  * http://www.firestormviewer.org
#  * $/LicenseInfo$
#
"""Rebrand user-visible XUI text, preserving IDs, links, protocols and license attribution."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
BRAND = 'ForMe.World'
NAMES = re.compile(r'(?<![\w])(?:OpenSim(?:ulator)?|Second[\s-]*Life|Fire[\s-]*storm)(?:\(tm\)|[®™])?(?![\w])', re.I)
TOKENS = re.compile(r'<!--[\s\S]*?-->|<!\[CDATA\[[\s\S]*?\]\]>|<(?:"[^"]*"|\'[^\']*\'|[^\'">])*>')
ATTR = re.compile(r'([\w.:-]+)(\s*=\s*)(["\'])(.*?)\3', re.S)
VISIBLE = {'label','label_selected','label_unselected','tool_tip','tooltip','title','short_title','text','description','message','watermark','window_title','accessible_label'}
PROTECTED = re.compile(r'(?:[a-z][a-z0-9+.-]*://[^\s<>"\']+|\b[\w.-]+@[\w.-]+\.[a-z]{2,}|\b(?:[\w-]+\.)+[a-z]{2,}(?:/[^\s<>"\']*)?|\[[^\]]*\])', re.I)
ATTRIBUTION = re.compile(r'copyright|\$LicenseInfo|GNU (?:General|Lesser)|Linden Research|The Phoenix Firestorm Project|Havok.*Physics|all rights reserved|free software|made.*source code available|Linden|Phoenix|contributed to|contributions from|Special thanks to|Starlight, modified', re.I)

def visible_text(text):
    # Attribution paragraphs identify upstream authors; they are not our product labels.
    if ATTRIBUTION.search(text): return text
    pieces=[]; last=0
    for match in PROTECTED.finditer(text):
        pieces.extend((NAMES.sub(BRAND,text[last:match.start()]),match.group()));last=match.end()
    pieces.append(NAMES.sub(BRAND,text[last:]))
    return ''.join(pieces)

def transform(source):
    pieces=[]; last=0
    for match in TOKENS.finditer(source):
        pieces.append(visible_text(source[last:match.start()]))
        tag=match.group()
        if tag.startswith('<!--'): pass
        elif tag.startswith('<![CDATA['): tag='<![CDATA['+visible_text(tag[9:-3])+']]>'
        else:
            def attr(m):
                value=visible_text(m.group(4)) if m.group(1) in VISIBLE else m.group(4)
                return m.group(1)+m.group(2)+m.group(3)+value+m.group(3)
            tag=ATTR.sub(attr,tag)
        pieces.append(tag);last=match.end()
    pieces.append(visible_text(source[last:]))
    return ''.join(pieces)

def files():
    yield from (ROOT/'indra/newview/skins').glob('*/xui/*/*.xml')
    yield from (ROOT/'indra/newview/skins').glob('*/skin.xml')
    yield from (ROOT/'indra/newview/skins').glob('*/themes/*/theme.xml')

def main(apply=False):
    changed=[]
    for path in sorted(set(files())):
        before=path.read_text(encoding='utf-8');after=transform(before)
        if before != after:
            changed.append(str(path.relative_to(ROOT)))
            if apply:path.write_text(after,encoding='utf-8')
    if not apply and changed:
        print('Unbranded UI text: '+'\n'.join(changed[:30]));return 1
    print(('Rebranded' if apply else 'PASS visible branding in')+f' {len(changed) if apply else len(set(files()))} XUI files')
    return 0
if __name__=='__main__':sys.exit(main('--apply' in sys.argv))
