"""Check packaged UI and protocol boundaries after removal of upstream features."""
from pathlib import Path
import plistlib
import xml.etree.ElementTree as ET
ROOT = Path(__file__).resolve().parents[2]
NEWVIEW = ROOT / 'indra/newview'
for name in ('llviewerhelp', 'llviewerhelputil', 'llfloaterhelpbrowser', 'llfloaterhowto', 'llfloaterhud'):
    for suffix in ('.cpp', '.h'):
        assert not (NEWVIEW / (name + suffix)).exists(), name
    assert name + '.cpp' not in (NEWVIEW / 'CMakeLists.txt').read_text(), name
removed = {'panel_preferences_colors.xml', 'panel_preferences_custom_colors.xml', 'floater_help_browser.xml', 'floater_how_to.xml', 'floater_hud.xml'}
for path in (NEWVIEW / 'skins').rglob('*.xml'):
    assert path.name not in removed, path
    tree = ET.parse(path)
    for node in tree.iter():
        if node.tag in ('menu', 'context_menu'):
            assert node.get('name') not in ('Help', 'Content'), path
        assert node.get('filename') not in removed, path
        assert node.get('function') not in ('ShowHelp', 'ToggleHelp', 'Help.ToggleHowTo'), path
        assert node.get('help_button') != 'true', path
plist = plistlib.loads((NEWVIEW / 'Info-Firestorm.plist').read_bytes())
assert [scheme for kind in plist['CFBundleURLTypes'] for scheme in kind['CFBundleURLSchemes']] == ['fmw']
manifest = (NEWVIEW / 'viewer_manifest.py').read_text()
for old in ('register_hopprotocol.sh', 'register_secondlifeprotocol.sh', 'handle_secondlifeprotocol.sh'):
    assert old not in manifest, old
assert 'register_fmwprotocol.sh' in manifest
assert 'ForMeAvatarBadges::updateSelf()' in (NEWVIEW / 'llappviewer.cpp').read_text()
assert 'ForMeAvatarBadge' in (NEWVIEW / 'llviewerregion.cpp').read_text()
assert 'id == gAgent.getID()' in (NEWVIEW / 'fsavatarbadges.h').read_text()
assert 'getCapability("ForMeAvatarBadge").empty()' in (NEWVIEW / 'fsavatarbadges.h').read_text()
assert 'secondlife:/app/' not in (NEWVIEW / 'llappviewermacosx.cpp').read_text()
print('PASS removed feature sources/resources, fmw packaging and own badge capability wiring')
