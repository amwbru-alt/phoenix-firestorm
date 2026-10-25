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
"""Regression tests for visible text changes without breaking protocol identifiers."""
import unittest
from branding import transform, visible_text

class BrandingTests(unittest.TestCase):
    def test_product_labels_and_notifications(self):
        self.assertEqual(transform('<string name="SecondLife">OpenSim / Second Life / FireStorm</string>'),
                         '<string name="SecondLife">ForMe.World / ForMe.World / ForMe.World</string>')
        self.assertEqual(transform('<panel name="OpenSim" label="Firestorm" control_name="OpenSim"/>'),
                         '<panel name="OpenSim" label="ForMe.World" control_name="OpenSim"/>')
    def test_protocol_links_placeholders_and_authors(self):
        source='secondlife://Alice https://www.firestormviewer.org/ [SecondLife] OpenSimExtras'
        self.assertEqual(visible_text(source),source)
        copyright='Copyright The Phoenix Firestorm Project, Inc.'
        self.assertEqual(visible_text(copyright),copyright)
        source='<string name="info"><![CDATA[OpenSim <b>Second Life</b>]]></string><!-- Firestorm -->'
        self.assertEqual(transform(source),'<string name="info"><![CDATA[ForMe.World <b>ForMe.World</b>]]></string><!-- Firestorm -->')
    def test_idempotence(self):
        source='<panel label="FireStorm"><text>Second Life</text></panel>'
        self.assertEqual(transform(transform(source)),transform(source))
if __name__=='__main__': unittest.main()
