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
"""Execute the actual standalone manifest naming methods without packaging dependencies."""
import ast
from pathlib import Path
import unittest

class PackagingBrandingTests(unittest.TestCase):
    def test_fixed_application_identity(self):
        source=Path(__file__).resolve().parents[2]/'indra/newview/viewer_manifest.py'
        module=ast.parse(source.read_text())
        manifest=next(node for node in module.body if isinstance(node,ast.ClassDef) and node.name=='ViewerManifest')
        names={'channel','app_name','friendly_app_name','installer_base_name','installer_base_name_mac','channel_type','channel_variant'}
        methods=[node for node in manifest.body if isinstance(node,ast.FunctionDef) and node.name in names]
        isolated=ast.Module(body=[ast.ClassDef(name='Manifest',bases=[],keywords=[],body=methods,decorator_list=[])],type_ignores=[])
        scope={'CHANNEL_VENDOR_BASE':'Firestorm'}
        exec(compile(ast.fix_missing_locations(isolated),str(source),'exec'),scope)
        instance=scope['Manifest']()
        instance.args={'channel':'Second Life','version':['1','2','3','4'],'arch':'x86_64'}
        self.assertEqual(instance.channel(),'ForMe.World')
        self.assertEqual(instance.app_name(),'ForMe.World')
        self.assertEqual(instance.friendly_app_name(),'ForMe.World')
        self.assertEqual(instance.channel_type(),'release')
        self.assertEqual(instance.installer_base_name(),'ForMe.World_1_2_3_4_x86_64')
        self.assertEqual(instance.installer_base_name_mac(),'ForMe.World_1_2_3_4_universal')
if __name__=='__main__': unittest.main()
