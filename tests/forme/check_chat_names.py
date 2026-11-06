#!/usr/bin/env python3
"""Regress the reported fmw agentself transcript URL and preserve sender labels."""
from pathlib import Path
import re
import subprocess
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / 'indra/llui/llurlentry.h').read_text()
macro = re.search(r'^#define APP_HEADER_REGEX\s+(.+)$', header, re.M).group(1)
# Exercise the exact production header expression with the same simple capture syntax.
probe = r'''
#include <regex>
#include <cassert>
#include <string>
#define APP_HEADER_REGEX MACRO
int main() {
 const std::string id="771b0f7e-5b22-49fc-be40-4ecc5be2cc53";
 for(const char* kind:{"agent","agentself"}) {
  std::regex pattern(std::string(APP_HEADER_REGEX)+"/"+kind+"/[\\da-f-]+/\\w+",std::regex::icase);
  for(const char* scheme:{"fmw:///app/","FMW:///app/","fmw://185.173.93.200:8002/app/"})
   for(const char* action:{"inspect","about","displayname","username"})
    assert(std::regex_match(std::string(scheme)+kind+"/"+id+"/"+action,pattern));
  assert(!std::regex_match(std::string("fmw:///app/")+kind+"//inspect",pattern));
  assert(!std::regex_match(std::string("https://outside.example/app/")+kind+"/"+id+"/inspect",pattern));
 }
}
'''.replace('MACRO', macro)
with tempfile.TemporaryDirectory() as directory:
    source = Path(directory) / 'agent_regex.cpp'
    binary = Path(directory) / 'agent_regex'
    source.write_text(probe)
    subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
chat = (ROOT / 'indra/newview/fschathistory.cpp').read_text()
assert 'appendText(std::string(name_params.link_href)' not in chat
assert 'name_params.is_link = true;' in chat
assert 'ForMeBadge::decorate(forme_badge, sender_label)' in chat
assert '!chat.mRlvNamesFiltered' in chat
assert 'FSChatHistoryShowYou' in chat
registry = (ROOT / 'indra/llui/llurlregistry.cpp').read_text()
assert registry.index('registerUrl(new FSUrlEntryAgentSelf()') < registry.index('registerUrl(new LLUrlEntryAgent()')
settings = list(ET.parse(ROOT / 'indra/newview/app_settings/settings.xml').getroot()[0])
index = next(i for i, node in enumerate(settings) if node.tag == 'key' and node.text == 'PlainTextChatHistory')
values = list(settings[index + 1])
value_index = next(i for i, node in enumerate(values) if node.tag == 'key' and node.text == 'Value')
assert values[value_index + 1].text == '0'
nearby = (ROOT / 'indra/newview/fsfloaternearbychat.cpp').read_text()
assert 'bool use_plain_text_chat_history = gSavedSettings.getBOOL("PlainTextChatHistory")' in nearby
print('PASS fmw agent/self name recognition, immediate clickable sender labels, privacy guard and selectable compact mode')
