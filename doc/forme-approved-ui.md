# Approved ForMe.World UI concepts

The approved concept sheets are implemented as native viewer surfaces:

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

## Communication and marketplace sheets (03–04)

- Direct messages retain native sessions, history, notifications, emoji and send
  callbacks. The left column shows actual portraits, recent message previews,
  timestamps and unread indicators, with conversation search. The right column
  has the current participant header and server badges when available. Message
  bubbles wrap again on resize; plain-text mode and special messages retain their
  original renderer. Original text stays in the document for history/search/copy.
- People has a portrait-based Nearby list with actual radar distances, profile,
  message and friendship actions. The detailed native radar remains available.
  Friends show known relationship status rather than claiming unknown avatars
  are offline. Search opens the real avatar picker.
- Groups has a separate window backed by the existing membership list, active
  group indication, group search and native create/profile/chat controls. Its
  localized resources retain the same native controls and callbacks.
- Market catalogue shows three real product cards per page, search, categories,
  per-account favorites and cart actions. Product detail shows actual photos,
  thumbnails, permissions, seller identity, configured currency, Buy and Add.
- The personal native cart has item rows, removal, estimated total and a server
  quote before explicit payment confirmation. It stores a durable request ID
  before sending checkout and retains it for uncertain outcomes. Buy submits
  only the selected product. Cart changes propagate to open native windows.
- The server companion advertises `ForMeMarketplace`, authenticates through the
  existing signed simulator bridge and reuses portal quote/checkout/delivery.
  It must be installed on both portal and region services before native commerce
  is usable. Until then, the client explains the missing capability and offers
  the website. This session has not deployed server changes. The browser cart
  and native cart have separate storage; they do not synchronize with each other.

## Validation

`python3 scripts/forme_preflight.py` runs native-control contracts, packaged asset
checks, layout checks and `tests/forme/test_approved_concepts.py`. The standalone marketplace policy probe also checks uncertain-payment recovery
and photo URL boundaries; `git diff --check` passes locally.
These checks do not compile the viewer or establish pixel agreement in the
running native client. The user has requested that client builds wait for an
explicit command. No new client build has been started for these changes.

The scenic background in the concept sheets is illustrative; these changes do
not replace the simulator scene, avatar appearance or renderer settings.
