"""Native marketplace boundaries; the server's executable tests cover its contract."""
from pathlib import Path
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
def read(path): return (ROOT/path).read_text()
adapter = read('indra/newview/fsmarketplacelistings.h')
assert 'ForMeCreatorAccess::allowed()' in adapter
assert 'getCapability("DirectDelivery")' in adapter and 'ForMeGrid::allowedServiceURI(cap)' in adapter
assert 'gSavedSettings' not in adapter
menu = read('indra/newview/llviewermenu.cpp')
start = menu.index('void LLSLMMenuUpdater::checkMerchantStatus(')
end = menu.index('void set_merchant_SLM_menu()', start)
assert 'ForMeMarketplace::available()' in menu[start:end]
assert 'isInSecondLife()' not in menu[start:end]
assert 'LLFloaterReg::hideInstance("marketplace_listings")' in menu[start:end]
app = read('indra/newview/llappviewer.cpp')
assert 'current_listing_cap != forme_listing_cap' in app and 'check_merchant_status(true)' in app
assert 'resetForMeSession()' in app and 'destroyInstance("marketplace_listings")' in app
market = read('indra/newview/llmarketplacefunctions.cpp')
assert market.count('if (!ForMeMarketplace::currentRequest(url)) return;') == 7
assert 'void LLMarketplaceData::resetForMeSession()' in market
assert 'mMarketplaceItems.clear();' in market and 'mPendingUpdateSet.clear();' in market
assert 'disconnect_all_slots();' in market
url_start = market.index('static std::string getMarketplaceURL(')
assert 'secondlife' not in market[url_start:market.index('// Get the version folder:', url_start)]
bridge = read('indra/newview/llinventorybridge.cpp')
assert bridge.count('&& !ForMeMarketplace::available()) return;') == 2
for path in (ROOT/'indra/newview/skins').glob('*/xui/*/menu_viewer.xml'):
    for node in ET.parse(path).getroot().iter():
        if node.get('name') == 'MarketplaceListings': assert node.get('forme_creator_only') == 'true', path
for path in (ROOT/'indra/newview/skins').glob('*/xui/*/floater_marketplace_listings.xml'):
    assert ET.parse(path).getroot().get('forme_creator_only') == 'true', path
print('PASS native marketplace principal/capability gates, region/role response binding, links and translated UI')

# A missing/failed merchant service must not hide the creator's menu entry.
menu_gate = menu[menu.index('void LLSLMMenuUpdater::setMerchantMenu()'):end]
assert 'setVisible(false)' not in menu_gate
assert 'setVisible(ForMeCreatorAccess::allowed())' in menu_gate
assert 'enableCommand(command->id(), ForMeCreatorAccess::allowed())' in menu_gate
floater = read('indra/newview/llfloatermarketplacelistings.cpp')
assert 'if (!ForMeCreatorAccess::allowed()) { closeFloater(); return; }' in floater
assert 'if (!ForMeMarketplace::available()) { updateView(); return; }' in floater
assert 'ForMeMarketplaceUnavailable' in floater
for language in ('en', 'ru'):
    strings = ET.parse(ROOT/f'indra/newview/skins/default/xui/{language}/strings.xml').getroot()
    for name in ('ForMeMarketplaceUnavailableTitle', 'ForMeMarketplaceUnavailable'):
        assert strings.find(f"./string[@name='{name}']") is not None
print('PASS creator marketplace entry survives missing service and merchant failures')

assert 'forme_creator != forme_listing_creator' in app
