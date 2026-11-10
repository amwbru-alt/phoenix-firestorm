#!/usr/bin/env python3
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
"""Build the native world-map skin's geometric UI assets (Pillow required)."""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / 'indra/newview/skins/default/textures/widgets'
S = 3


def surface(w, h):
    return Image.new('RGBA', (w*S, h*S))


def save(name, im):
    im.resize((im.width//S, im.height//S), Image.Resampling.LANCZOS).save(DEST / (name+'.png'))


def rounded(name, w, h, radius, top, bottom, outline=None, line=1):
    im = surface(w, h)
    d = ImageDraw.Draw(im)
    for y in range(h*S):
        t = y/max(1, h*S-1)
        color = tuple(round(a+(b-a)*t) for a,b in zip(top,bottom))
        d.line((0,y,w*S,y), fill=color)
    mask = Image.new('L', im.size)
    ImageDraw.Draw(mask).rounded_rectangle((0,0,w*S-1,h*S-1), radius*S, fill=255)
    im.putalpha(mask)
    if outline:
        d.rounded_rectangle((line*S/2,line*S/2,w*S-1-line*S/2,h*S-1-line*S/2),
                            max(0,radius*S-line*S/2), outline=outline, width=line*S)
    save(name,im)


def main():
    rounded('Forme_WMap_Window',746,450,22,(59,59,58),(48,47,46), (89,86,83))
    rounded('Forme_WMap_Field',390,40,16,(64,64,63),(60,60,59),(91,90,88))
    rounded('Forme_WMap_Switch',294,40,20,(58,58,57),(54,54,53),(87,85,82))
    rounded('Forme_WMap_Accent',148,40,20,(170,129,208),(161,119,201))
    rounded('Forme_WMap_Teleport',266,44,14,(170,127,207),(159,114,198))
    rounded('Forme_WMap_Copy',266,44,14,(58,57,56),(54,53,52),(139,133,128))
    rounded('Forme_WMap_Card',225,305,17,(60,59,57),(51,49,47),(85,81,77))
    # Transparent ring, deliberately outside the complete selected card.
    im=surface(233,313); d=ImageDraw.Draw(im)
    d.rounded_rectangle((2*S,2*S,231*S,311*S),20*S,outline=(188,138,236),width=3*S)
    save('Forme_WMap_Selected',im)
    # Rounded photo masks: only corners cover the image underneath.
    for name,w,h,r in [('Card',225,190,17),('Photo',276,118,16),('Map',412,305,16)]:
        im=Image.new('RGBA',(w*S,h*S),(54,53,51,255))
        cut=Image.new('L',im.size,255)
        ImageDraw.Draw(cut).rounded_rectangle((0,0,w*S-1,h*S-1),r*S,fill=0)
        im.putalpha(cut); save('Forme_WMap_Mask_'+name,im)
    im=surface(40,36); d=ImageDraw.Draw(im)
    d.rounded_rectangle((0,0,40*S,36*S),8*S,fill=(40,40,39,220))
    save('Forme_WMap_Zoom',im)
    im=surface(24,24); d=ImageDraw.Draw(im)
    for x,y,r in [(9,7,3),(17,8,2.5)]:
        d.ellipse(((x-r)*S,(y-r)*S,(x+r)*S,(y+r)*S),outline=(222,220,218),width=S)
    d.rounded_rectangle((3*S,13*S,15*S,21*S),3*S,outline=(222,220,218),width=S)
    d.arc((13*S,13*S,22*S,22*S),245,80,fill=(222,220,218),width=S)
    save('Forme_WMap_People',im)
    im=surface(48,60); d=ImageDraw.Draw(im)
    d.ellipse((1*S,47*S,47*S,59*S),fill=(179,117,226,100),outline=(197,149,236,190),width=2*S)
    d.polygon([(10*S,27*S),(24*S,52*S),(38*S,27*S)],fill=(174,117,222))
    d.ellipse((7*S,4*S,41*S,38*S),fill=(174,117,222),outline=(202,160,237),width=S)
    d.ellipse((15*S,12*S,33*S,30*S),fill=(255,255,255))
    d.ellipse((20*S,17*S,28*S,25*S),fill=(174,117,222))
    save('Forme_WMap_Pin',im)
    im=surface(20,20); d=ImageDraw.Draw(im)
    d.line((4*S,4*S,16*S,16*S),fill=(221,218,215),width=S)
    d.line((16*S,4*S,4*S,16*S),fill=(221,218,215),width=S)
    save('Forme_WMap_Close',im)
    im=surface(24,24); d=ImageDraw.Draw(im)
    points=[(3,6),(9,3),(16,6),(22,3),(22,19),(16,22),(9,19),(3,22),(3,6)]
    d.line([(x*S,y*S) for x,y in points],fill=(236,232,240),width=S,joint='curve')
    for x,y1,y2 in [(9,3,19),(16,6,22)]: d.line((x*S,y1*S,x*S,y2*S),fill=(236,232,240),width=S)
    save('Forme_WMap_Map',im)
    im=surface(24,24); d=ImageDraw.Draw(im)
    for x,y in [(3,3),(14,3),(3,14),(14,14)]:
        d.rounded_rectangle((x*S,y*S,(x+7)*S,(y+7)*S),S,outline=(236,232,240),width=S)
    save('Forme_WMap_Cards',im)
    im=surface(30,30); d=ImageDraw.Draw(im)
    d.arc((8*S,2*S,22*S,16*S),180,360,fill=(235,232,230),width=S)
    d.line((8*S,9*S,8*S,22*S),fill=(235,232,230),width=S)
    d.line((22*S,9*S,22*S,20*S),fill=(235,232,230),width=S)
    d.arc((8*S,15*S,18*S,29*S),0,180,fill=(235,232,230),width=S)
    d.line((18*S,22*S,18*S,10*S),fill=(235,232,230),width=S)
    d.arc((12*S,7*S,18*S,13*S),180,360,fill=(235,232,230),width=S)
    d.line((12*S,10*S,12*S,22*S),fill=(235,232,230),width=S)
    save('Forme_WMap_Link',im.rotate(-40,resample=Image.Resampling.BICUBIC).resize((24*S,24*S),Image.Resampling.LANCZOS))
    im=surface(1,1); save('Forme_WMap_Clear',im)


if __name__ == '__main__':
    main()
