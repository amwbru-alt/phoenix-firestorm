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
"""Annotate creator-only UI by action IDs, preserving translations and other UI state."""
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET
ROOT=Path(__file__).resolve().parents[2]
# Whole tools/editor windows; appearance and ordinary inventory usage are separate.
WINDOWS={'floater_marketplace_listings.xml','floater_associate_listing.xml','floater_marketplace_validation.xml','floater_tools.xml','floater_build_options.xml','floater_bulk_upload.xml','floater_model_preview.xml',
         'floater_import_collada.xml','floater_fs_import.xml','floater_material_editor.xml',
         'floater_live_material_editor.xml','floater_particle_editor.xml','floater_script.xml',
         'floater_script_preview.xml','floater_script_queue.xml','floater_script_recover.xml',
         'floater_live_lsleditor.xml','floater_fs_export.xml','floater_animation_bvh_preview.xml','floater_image_preview.xml','floater_sound_preview.xml','floater_animation_anim_preview.xml','panel_snapshot_inventory.xml'}
ACTIONS={'Object.Delete','Tools.SaveToObjectInventory','Tools.SnapObjectXY','Object.Take','Object.TakeSeparate','Object.Return','Object.Build','Object.Edit','Object.EditGLTFMaterial','Object.EditParticles','Object.Duplicate',
         'Object.Export','Object.ExportCollada','Object.TakeSeparateCopy','Land.Build','Land.Edit','Build.Toggle',
         'Tools.Link','Tools.Unlink','Tools.EditLinkedParts','Tools.TakeCopy','Tools.SelectedScriptAction',
         'Tools.SelectTool','Advanced.ClickGLTFUpload','Advanced.ClickGLTFEdit','Admin.ForceTakeCopy',
         'Inventory.Share','Avatar.Share','InspectAvatar.Share','Minimap.Share','Attachment.Edit'}
PARENTS={'MarketplaceListings','BuildTools','Upload','upload','upload_options','upload_def','Export Menu'}
FLOATER_IDS={'build','build_options','bulk_upload','upload_model','import','fs_import','material_editor',
             'live_material_editor','particle_editor','preview_script','script_recover'}
# Explicit transfer and content-authoring buttons. No generic edit/copy/appearance-name matching.
BUTTON_NAMES={'share_btn','share_button','btn_share','give_btn','give_button','build_btn','build_button','share_btn_lp','upload_raw_btn','bake_terrain_btn'}
INVENTORY_CREATE={'lsl','script','notecard','gesture','material','pbr_material'}
TOKEN=re.compile(r'<!--[\s\S]*?-->|<!\[CDATA\[[\s\S]*?\]\]>|<(?:"[^"]*"|\'[^\']*\'|[^\'">])*>')
def restricted_action(function,parameter):
    if function == 'Inventory.DoToSelected' and (parameter.startswith('marketplace_') or parameter in {'copy_to_marketplace_listings','move_to_marketplace_listings'}): return True
    if function.startswith('Admin.HandleObject') or function=='Admin.HandleForceDelete':return True
    if function in ACTIONS or function.startswith('File.Upload') or function=='File.ImportLinkset':return True
    if function=='Inventory.FileUploadLocation' and parameter.startswith('upload_'):return True
    if function=='Inventory.DoCreate' and parameter in INVENTORY_CREATE:return True
    if function in {'Avatar.DoToSelected','Avatar.GearDoToSelected','Profile.Commit','AvatarIcon.Action','Calllog.Action','Inventory.GearDefault.Custom.Action','Places.LandmarksGear.Custom.Action'} and parameter=='share':return True
    if function=='AreaSearch.Action' and parameter=='edit':return True
    if function.startswith('Floater.') and parameter in FLOATER_IDS:return True
    return False

def files():return sorted((ROOT/'indra/newview/skins').glob('*/xui/*/*.xml'))
def keys():
    restricted=set()
    for path in files():
        root=ET.parse(path).getroot()
        for node in root.iter():
            name=node.get('name','')
            callbacks=[child for child in node if child.tag.endswith(('on_click','on_commit'))]
            is_menu=node.tag in {'menu','context_menu','menu_item_call','menu_item_check'}
            mark=(is_menu and name in PARENTS) or ((node.tag in {'button','flyout_button'} and name.lower() in BUTTON_NAMES) or (path.name=='panel_profile_secondlife.xml' and name in {'can_edit_objects','cant_edit_objects'}) or (path.name in {'floater_item_properties.xml','floater_inventory_item_properties.xml','floater_task_properties.xml'} and name in {'CheckShareWithGroup','CheckEveryoneCopy','CheckNextOwnerModify','CheckNextOwnerCopy','CheckNextOwnerTransfer','CheckOwnerExport','CheckPurchase','ComboBoxSaleType','Edit Cost'}))
            mark=mark or any(restricted_action(child.get('function',''),child.get('parameter','')) for child in callbacks)
            if mark:restricted.add((path.name,node.tag,name))
    return restricted

def transform(path,source,restricted):
    first=True
    def replace(match):
        nonlocal first
        tag=match.group()
        if tag.startswith(('<!--','<!','<?','</')):return tag
        opening=tag[:-2] if tag.endswith('/>') else tag[:-1]
        node=ET.fromstring(opening+'/>')
        mark=(first and path.name in WINDOWS) or (path.name,node.tag,node.get('name','')) in restricted
        first=False
        if not mark or node.get('forme_creator_only')=='true':return tag
        assert 'forme_creator_only' not in node.attrib,path
        end='/>' if tag.endswith('/>') else '>'
        return tag[:-len(end)]+' forme_creator_only="true"'+end
    return TOKEN.sub(replace,source)
def main(apply=False):
    restricted=keys();changed=[]
    for path in files():
        before=path.read_text();after=transform(path,before,restricted)
        if before!=after:
            changed.append(str(path.relative_to(ROOT)))
            if apply:path.write_text(after)
    if changed and not apply:
        print('Missing creator UI gates: '+'\n'.join(changed[:30]));return 1
    print(('Annotated' if apply else 'PASS creator UI coverage in')+f' {len(changed) if apply else len(files())} XUI files')
    return 0
if __name__=='__main__':sys.exit(main('--apply' in sys.argv))
