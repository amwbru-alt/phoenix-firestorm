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
market = read('indra/newview/llmarketplacefunctions.cpp')
assert market.count('if (!ForMeMarketplace::currentRequest(url)) return;') == 7
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
