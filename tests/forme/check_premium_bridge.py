from pathlib import Path
import xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[2]
for path in (ROOT/'indra/newview/skins').glob('*/xui/*/menu_viewer.xml'):
    ET.parse(path)
    text=path.read_text()
    assert 'Membership.UpdateLabel' not in text,path
    assert 'secondlife.com/my/account/membership.php' not in text,path
source=(ROOT/'indra/newview/skins/default/xui/en/menu_viewer.xml').read_text()
assert 'http://185.173.93.200:8002/web/premium.html' in source
assert 'LLUpdateMembershipLabel' not in (ROOT/'indra/newview/llviewermenu.cpp').read_text()
assert 'std::string currency = "₽"' in (ROOT/'indra/newview/llstartup.cpp').read_text()
assert 'Tea::setRegionCurrency("₽")' in (ROOT/'indra/newview/lfsimfeaturehandler.cpp').read_text()
print('PASS own Premium entry point, translated menus, ruble display and no legacy membership callback')
