# ForMe.World creator-only client controls

Only the exact server badge IDs `owner` and `designer` can use content-authoring
and item-transfer functions in this distribution. Administrators, moderators,
residents and future role IDs are denied. Labels, colors, saved settings, friend
rights and god mode cannot supply this access. Existing simulator ownership,
parcel and item-permission checks still apply to eligible creators.

The policy reads the authenticated agent's avatar NameValue metadata through the
versioned, fixed-server badge adapter. Login must be complete, the avatar must be
alive and in the agent's current region. Missing, invalid or unsupported metadata
means no access. Another avatar's owner/designer badge never authorizes the local
user.

## Interface

`forme_creator_only="true"` is an inherited LLView parameter. Both visibility and
input availability are gated by an in-memory state derived from server metadata;
it is not a debug setting. The requested visible/enabled state is retained when
access is denied, so a later grant or revocation takes effect without rebuilding
the UI. Menu/button visibility callbacks cannot override the role gate.

All shipped skins/translations are annotated by stable action and control IDs.
Coverage includes the Build menu and toolbar button, object/land/attachment edit
menus, upload/import dialogs, script/material/particle editors, sharing through
profiles/IM/people/radar/inventory, object permission/sale controls and terrain
upload controls. `tests/forme/creator_ui.py --check` detects missing annotations
for these actions; `--apply` updates them without rewriting translations.

## Execution checks

Checks also run before upload entry points, file-picker callbacks, mesh/material
upload, linkset import stages, primitive creation, object transforms/properties,
link/unlink/delete/take/return, terrain modification, physical-object dragging,
script editing/queue confirmations and the final item/folder give operations.
Revoked creators are switched out of active manipulation tools. New internal
bridge object/script creation is also restricted.

This preserves ordinary movement, camera use, touching objects, inventory browsing,
wearing/detaching existing clothes/attachments and avatar appearance baking.
Outgoing item/folder transfers are restricted; receiving items is not globally
disabled. Profile text/photo editing and ordinary personal outfit organization
are not creator tools.

Role checks are repeated at subsequent action/confirmation boundaries. Requests
already sent to the simulator cannot be recalled by the UI. Already-open creator
windows are hidden when access is revoked. No role ID is inferred from chat or a
manually typed display-name prefix.

## Scope and validation

This is an interface and action restriction in this client. It is not a server
authorization boundary. The simulator and asset services must enforce equivalent
rules to reject other/modified viewers and scripted HTTP/protocol requests. This
change does not alter the server or claim that avatar badge rendering itself
grants simulator privileges.

The native policy test covers exact role matching, invalid metadata, spoofed
labels, unknown roles, startup denial and grant/revocation transitions. Structural
checks cover current-agent provenance, action/confirmation guards, dynamic UI
state, all shipped XUI files, and preservation of normal gameplay controls.
The workflow also retains existing fixed-grid, badge, branding and packaging
regression checks.

A full viewer build and in-world visual/interaction checks are still required.
Before distributing an installer, test owner, designer, administrator, moderator,
no badge, malformed/unknown role, live grant/removal, region crossing, old file
pickers and give confirmations, and normal camera/touch/wear/baking workflows.
