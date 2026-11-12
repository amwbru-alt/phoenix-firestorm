# Approved ForMe.World UI concepts

The two approved concept sheets are implemented as native viewer surfaces:

- Header and tools row: map, people, inventory, marketplace, creator tools,
  region search and preferences. The tools row collapses on narrow windows;
  the floating dock remains available. The account menu has three actions.
- Compact profile: circular live portrait, identity, authoritative server badge,
  About/Places tabs and message/details actions. The full native editor remains
  available through the ellipsis button as `profile_details`. It uses a separate
  copy of the native profile form, including translated overlays.
- Inventory: larger rows and icons, folder navigation, compact view selector,
  native item context actions and existing category, search and selection logic.
  The view selector also includes restored custom inventory tabs.
- Preferences: six primary navigation buttons, advanced sections in a dropdown,
  a settings-backed interface page and Apply/Cancel. The hidden full-menu binding
  participates in the native preferences snapshot so Cancel can restore it.
- Camera: three main presets, line-rendered orbit/pan controls, zoom, retained
  camera presets, walk/run/fly actions routed through native movement handlers,
  state indication and reset.
- Notifications: request/notification tabs, corrected expanding list bounds,
  inline native responses for simple friendship/teleport offers, unread filtering
  and mark-read for the current viewer session. Marking read does not cancel or
  accept a notification. Offers involving fees or special warnings retain their
  full native confirmation dialog. Tagged list registration is deduplicated so
  counters and iteration do not repeat the same list for multiple request tags.

## Validation

`python3 scripts/forme_preflight.py` runs native-control contracts, packaged asset
checks, layout checks and `tests/forme/test_approved_concepts.py`. Grid and creator
integration checks, CI-validation tests and `git diff --check` also pass locally.
These checks do not compile the viewer or establish pixel agreement in the
running native client. The user has requested that client builds wait for an
explicit command. No new client build has been started for these changes.

The scenic background in the concept sheets is illustrative; these changes do
not replace the simulator scene, avatar appearance or renderer settings.
