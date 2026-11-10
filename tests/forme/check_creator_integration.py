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
"""Structural action/UI checks; full viewer compilation and in-world QA are separate."""
from pathlib import Path
import re
import xml.etree.ElementTree as ET
from creator_ui import ROOT, files, keys, transform

def read(path): return (ROOT/path).read_text()
def guard(path, method):
    s=read('indra/newview/'+path)
    pattern=re.compile(r'^(?:bool|void|LLUUID|EAcceptance|S32)\s+'+re.escape(method)+r'\s*\([^;{}]*?\)\s*(?:const\s*)?(?://[^\n]*\n\s*)*\{',re.M)
    found=list(pattern.finditer(s));assert found,(path,method)
    for m in found:
        body=s[m.end():].lstrip()
        assert body.startswith('if (!ForMeCreatorAccess::allowed()) return'),(path,method,body[:100])
for path,methods in {
 'llgiveinventory.cpp':['LLGiveInventory::commitGiveInventoryItem','LLGiveInventory::commitGiveInventoryCategory'],
 'llavataractions.cpp':['LLAvatarActions::share','LLAvatarActions::shareWithAvatars','LLAvatarActions::canShareSelectedItems'],
 'llviewermenufile.cpp':['upload_single_file','upload_bulk','upload_new_resource'],
 'llfloatermodelpreview.cpp':['LLFloaterModelPreview::onUpload','LLFloaterModelPreview::isModelUploadAllowed'],
 'llmeshrepository.cpp':['LLMeshRepository::uploadModel'],
 'lltoolplacer.cpp':['LLToolPlacer::addObject','LLToolPlacer::addDuplicate'],
 'lltoolmgr.cpp':['LLToolMgr::canEdit','LLToolMgr::toggleBuildMode'],
 'lltooldraganddrop.cpp':['LLToolDragAndDrop::handleGiveDragAndDrop','LLToolDragAndDrop::dropObject','LLToolDragAndDrop::dropInventory','LLToolDragAndDrop::dropTexture','LLToolDragAndDrop::dropMaterial','LLToolDragAndDrop::dropScript'],
 'llselectmgr.cpp':['LLSelectMgr::sendMultipleUpdate','LLSelectMgr::sendOwner','LLSelectMgr::sendLink','LLSelectMgr::sendDelink','LLSelectMgr::selectionSetObjectSaleInfo','LLSelectMgr::sendSelectionMove'],
 'llviewermenu.cpp':['handle_object_edit','handle_object_edit_gltf_material','handle_take','handle_take_copy','handle_object_return'],
 'llpreviewscript.cpp':['LLPreviewLSL::saveIfNeeded','LLLiveLSLEditor::saveIfNeeded'],
 'llcompilequeue.cpp':['LLFloaterScriptQueue::onScriptModifyConfirmation','LLFloaterCompileQueue::processScript'],
 'fsfloaterimport.cpp':['FSFloaterImport::createPrim','FSFloaterImport::uploadAsset'],
 'lltoolbrush.cpp':['LLToolBrushLand::modifyLandAtPointGlobal','LLToolBrushLand::modifyLandInSelectionGlobal'],
 'lltoolgrab.cpp':['LLToolGrabBase::handleHoverActive'],
 'llfloaterproperties.cpp':['LLFloaterProperties::onCommitPermissions','LLFloaterProperties::updateSaleInfo'],
}.items():
    for method in methods:guard(path,method)
adapter=read('indra/newview/fscreatoraccess.h')
for item in ('STATE_STARTED','gAgent.getID()','self->getRegion() != region','self->isDead()','ForMeAvatarBadges::get(gAgent.getID())','ForMeCreator::allowed(badge.valid, badge.id)'):assert item in adapter,item
assert 'gSavedSettings' not in adapter and 'isGodlike' not in adapter
view=read('indra/llui/llview.h')
assert 'sForMeCreatorAccess = false' in view
assert 'return mEnabled && forMeCreatorVisible()' in view
assert 'return forMeCreatorVisible() ? mVisible : hidden' in view
view_cpp=read('indra/llui/llview.cpp')
assert 'setEnabled(mEnabled);' in view_cpp and 'setVisible(mVisible);' in view_cpp
assert 'setEnabled(getEnabled());' not in view_cpp # do not persist temporary role denial at widget creation
assert 'button_p.forme_creator_only = commandp->name() == "build"' in read('indra/llui/lltoolbar.cpp')
assert 'LLView::setForMeCreatorAccess(!gDisconnected && ForMeCreatorAccess::allowed())' in read('indra/newview/llappviewer.cpp')
assert 'cur_tool = LLToolPie::getInstance();' in read('indra/newview/lltoolmgr.cpp')
restricted=keys();count=0
for path in files():
    source=path.read_text();assert transform(path,source,restricted)==source,path
    root=ET.fromstring(source)
    count+=sum(node.get('forme_creator_only')=='true' for node in root.iter())
# Normal gameplay controls and appearance are outside the creator-only gate.
for filename,action in [('menu_avatar_self.xml','EditShape'),('menu_object.xml','Object.Touch'),('menu_attachment_self.xml','Attachment.Detach')]:
    root=ET.parse(ROOT/'indra/newview/skins/default/xui/en'/filename).getroot()
    found=False
    for node in root.iter():
        if any(child.get('function')==action for child in node):
            found=True;assert node.get('forme_creator_only')!='true',(filename,action)
    assert found,(filename,action)
# Appearance baking has its own upload path and must keep working for residents.
assert 'ForMeCreatorAccess' not in read('indra/newview/llviewertexlayer.cpp')
print(f'PASS creator identity, mutation/confirmation guards, runtime visibility, gameplay preservation and {count} UI gates')
