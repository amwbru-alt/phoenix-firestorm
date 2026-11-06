#!/usr/bin/env python3
"""Render editable ForMe line icons into the viewer's native clipped atlas (requires sharp)."""
from pathlib import Path
import json
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'indra/newview/skins/default/textures'
roles = json.loads((BASE / 'forme_ui/icons.json').read_text())
aliases = json.loads((BASE / 'forme_ui/aliases.json').read_text())
jobs = {}
for filename, role, width, height, clip in aliases.values():
    key = (role, width, height)
    jobs[key] = {
        'left': clip['left'], 'top': 1024 - clip['top'],
        'svg': f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
               'viewBox="-1 -1 26 26" fill="none" stroke="#f1eef3" stroke-width="1.6" '
               f'stroke-linecap="round" stroke-linejoin="round">{roles[role]}</svg>'
    }
with tempfile.TemporaryDirectory() as temp:
    manifest = Path(temp) / 'icons.json'
    manifest.write_text(json.dumps(list(jobs.values())))
    source = '''const fs=require('fs');
const paths=process.env.CODEX_PRIMARY_RUNTIME_NODE_MODULES?[process.env.CODEX_PRIMARY_RUNTIME_NODE_MODULES]:[];
const sharp=require(require.resolve('sharp',{paths:[process.cwd(),...paths]}));
const jobs=JSON.parse(fs.readFileSync(process.argv[1]));
(async()=>{
 const layers=await Promise.all(jobs.map(async j=>({input:await sharp(Buffer.from(j.svg)).png().toBuffer(),left:j.left,top:j.top})));
 await sharp({create:{width:1024,height:1024,channels:4,background:{r:0,g:0,b:0,alpha:0}}}).composite(layers).png().toFile(process.argv[2]);
})().catch(e=>{console.error(e);process.exit(1)});
'''
    subprocess.run([os.environ.get('NODE', 'node'), '-e', source, str(manifest),
                    str(BASE / 'forme_ui/Icons.png')], cwd=ROOT, check=True)
print(f'Rendered {len(jobs)} native-size sprites in Icons.png')
