from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
source = (ROOT/'indra/newview/fsdata.cpp').read_text()
app = (ROOT/'indra/newview/llappviewer.cpp').read_text()
header = (ROOT/'indra/newview/fsdata.h').read_text()
for old in ('phoenixviewer.com/app/', 'callbackHttpGetRaw', 'fsdata.xml', 'agents.xml', 'assets.xml', 'defaults.xml', 'BlockedReleases', 'mTeamAgents', 'saveLLSD', 'loadFromFile'):
    assert old not in source, old
assert 'fsdata_defaults' not in app
assert 'ForMeViewerData::Endpoint' in source
assert 'setFollowRedirects(false)' in source
assert 'setTimeout(5)' in source and 'setRetries(0)' in source
assert 'getFSDataDone() const { return true; }' in header
assert 'getAgentsDone() const { return true; }' in header
assert 'getSystemInfo()' in source
assert 'ForMeViewerData::valid' in source
print('PASS own asynchronous viewer metadata, no vendor caches/downloads, no login dependency')
