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
"""Structural integration contracts; full viewer builds remain a separate check."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
def source(path): return (ROOT / path).read_text(encoding='utf-8')
def body(path, signature):
    s=source(path); start=s.index('{',s.index(signature)); depth=1; i=start+1
    # Target policy methods have no brace-bearing string literals.
    while depth:
        depth += (s[i]=='{')-(s[i]=='}'); i+=1
    return s[start:i]

manager='indra/newview/fsgridhandler.cpp'
init=body(manager,'void LLGridManager::initGrids()')
assert 'initGridList(' not in init and 'initCmdLineGrids(' not in init
assert 'ForMeGrid::LoginURI' in init and 'GP_OPENSIM' in init
for signature in ['void LLGridManager::addGrid(const std::string&', 'void LLGridManager::addGrid(GridEntry*', 'void LLGridManager::initGridList(', 'void LLGridManager::reFetchGrid(']:
    value=body(manager,signature)
    assert 'HttpCoroutineAdapter' not in value and 'FETCH)' not in value and 'mGridList[' not in value
assert 'ForMeGrid::LoginURI' in body(manager,'void LLGridManager::getLoginURIs(')
assert 'ForMeGrid::allowedGrid' in body(manager,'void LLGridManager::setGridChoice(')
assert 'return {{ForMeGrid::GridID, ForMeGrid::Label}}' in body(manager,'std::map<std::string, std::string> LLGridManager::getKnownGrids()')
assert 'CmdLineHelperURI' not in body(manager,'std::string LLGridManager::getHelperURI()')
assert 'LoginPage' not in body(manager,'std::string LLGridManager::getLoginPage()')[1:-1].split('return')[0]
login=body('indra/viewer_components/login/lllogin.cpp','void LLLogin::Impl::loginCoro(')
assert login.index('allowedLoginURI(uri)') < login.index('LLEventPumps::instance()')
assert login.index('allowedLoginURI(request["uri"]') < login.index('llcoro::postAndSuspend(')
assert 'allowedSimulator(response["sim_ip"]' in login and 'allowedServiceURI(response["seed_capability"]' in login
xmlrpc=source('indra/newview/llxmlrpctransaction.cpp')
assert 'setFollowRedirects(false)' in xmlrpc and 'method == "login_to_simulator" && !ForMeGrid::allowedLoginURI(uri)' in xmlrpc
assert 'bool show_server = false;' in source('indra/newview/fspanellogin.cpp')
assert 'ForMeGrid::allowedCredential(saved)' in source('indra/newview/llloginhandler.cpp')
assert 'ForMeGrid::allowedGrid(queryMap["grid"].asString())' in source('indra/newview/llloginhandler.cpp')
for path in ['indra/newview/llstartup.cpp','indra/newview/llurldispatcher.cpp']:
    assert 'ForMeGrid::localRegion(slurl.getRegion())' in source(path)
for path in ['indra/newview/llworld.cpp','indra/newview/llviewermessage.cpp']:
    assert 'ForMeGrid::allowedSimulator(' in source(path)
assert 'ForMeGrid::allowedServiceURI(url)' in source('indra/newview/llviewerregion.cpp')
for path in ROOT.glob('indra/newview/skins/*/xui/*/floater_preferences.xml'):
    root=ET.parse(path).getroot()
    assert not root.findall('.//panel[@name="opensim"]'),path
for path in ROOT.glob('indra/newview/skins/*/xui/*/panel_login*.xml'):
    root=ET.parse(path).getroot()
    for control in root.iter():
        if control.get('name')=='server_combo': assert control.get('enabled')=='false',path
        if control.get('name')=='grid_panel': assert control.get('visible')=='false',path
catalog=ET.parse(ROOT/'indra/newview/app_settings/grids.xml').getroot().find('map')
assert [e.text for e in catalog.findall('key')]==['185.173.93.200:8002']
cmake=source('indra/CMakeLists.txt')
assert 'set(OPENSIM ON CACHE BOOL' in cmake and 'set(SINGLEGRID ON CACHE BOOL' in cmake
assert 'set(HAVOK_TPV OFF CACHE BOOL' in cmake
assert cmake.index('set(OPENSIM ON CACHE BOOL') < cmake.index('include(Variables)')
print('PASS fixed-grid manager, authentication/redirect boundaries, saved credentials, simulator handoffs, XUI skins and build contracts')
