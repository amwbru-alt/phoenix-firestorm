# ForMe.World viewer identity and avatar badges

This distribution connects only to http://185.173.93.200:8002/. Product labels,
notifications, settings descriptions, skin labels and native application titles
use ForMe.World across the shipped translations. Executable/resource paths,
settings identifiers, URI schemes, service field names and upstream attribution
remain compatible. User messages, avatar names and object names are not rewritten.

## Server contract

Compatible with SERVER/doc/AvatarBadges.md on the server's
pr-56-review-fixes-20261013 branch. The simulator advertises a top-level
`ForMeAvatarBadges` map with integer `version: 1` in SimulatorFeatures.
Each avatar's ObjectUpdate NameValue list contains all five STRING fields:

```
ForMeBadgeId STRING RW SV owner
ForMeBadgeLabel STRING RW SV Владелец
ForMeBadgeColor STRING RW SV #6D28D9
ForMeNameColor STRING RW SV #6D28D9
ForMeChatColor STRING RW SV #6D28D9
```

| Default role | Label | Badge/name/chat color |
| --- | --- | --- |
| owner | Владелец | #6D28D9 |
| administrator | Администратор | #D4AF37 |
| moderator | Модератор | #38BDF8 |
| designer | Дизайнер | #22C55E |

The viewer has no hardcoded role roster. Adding a valid role or changing its
colors on the server requires no viewer rebuild. Badge and name colors must
match; chat color can differ. IDs are lowercase ASCII slugs of at most 32
characters; labels are valid UTF-8 of at most 32 code points, without controls,
markup or bidirectional formatting. Colors use #RRGGBB.

The viewer adds `[Label] ` beside the name and colors both equally. It supports
native metadata mode and the server's legacy prefix mode without adding the same
prefix twice. Rendering covers avatar nametags, expanded and compact chat in both
history widgets, chat bubbles, and nearby/IM consoles. The console retains
separate name and body color segments through wrapping, resizing and URL-label
callbacks. Chat fade/distance alpha still applies.

Only simulator-owned metadata attached to an actual avatar object on the fixed
server is accepted. Scripted objects, typed role prefixes, missing metadata,
unsupported protocol versions and malformed fields do not confer role styling.
Muted, anonymized and replayed history messages do not receive verified badge
styling. The renderer does not grant permissions.

The ObjectUpdate list is authoritative: replacement without the five fields
revokes the badge. NameValue changes invalidate nametags; SimulatorFeatures
updates invalidate them on the main loop. Chat header grouping separates changed
roles. Already rendered messages retain their appearance as historical messages;
subsequent messages use current metadata. There is no persistent badge cache.

Metadata is available for avatars currently known to the viewer in the region;
offline/remote IM senders without an avatar object display normal names/colors.
The advertised catalog is not a UUID roster, so the viewer does not invent an
unsupported global badge lookup.

## Validation

The ForMe workflow compiles and executes the production grid policy and badge
parser tests. Python checks cover rendering integration boundaries, all XUI XML,
visible branding, preserved technical identifiers/attribution, and application
packaging names. These tests are not a full viewer build or an in-world visual
test. Before distributing an installer, build the complete viewer and check the
four roles plus a custom role, role removal, legacy/native prefix modes, both
chat layouts, console URL resolution, region crossings and privacy restrictions
against the configured server.

Creator-only client controls are documented in [FORME_CREATOR_ACCESS.md](FORME_CREATOR_ACCESS.md).
