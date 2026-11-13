# ForMe.World world protocol v1

`forme-world-v1.schema.json` is copied unchanged to the server and viewer repositories. The active capability is `ForMeWorld`; its random URL is registered before avatar presence creation, bound to the authenticated login session, and usable only by a live root presence.

POST requests:

- `{"action":"info"}` returns the active backend and implemented feature flags.
- `{"action":"poll","epoch":"","after":"0"}` starts a bounded interest subscription. On later polls, send the returned epoch and decimal cursor. A repeated unacknowledged cursor replays the previous page. A new epoch/reset replaces the entire local projection. A tombstone also means leaving interest, not necessarily deleting the actual world object. Clients reject cursor gaps, stale revisions and partial invalid pages.
- `{"action":"material","entityId":"UUID","expectedRevision":"N","material":{...}}` updates only the physical properties implemented by the active backend, after server role, additional creator grant, object and land checks. It does not advance the polling cursor. Read the next replica revision before another edit.

Region coordinates are metres, Z-up; the existing map grid origin is `(gridX * 256, gridY * 256, 0)`. All revisions, authority epochs and cursors are decimal nonnegative int64 strings. Session/node IDs are UUIDs. The current server projection is limited to 200 nearest entities; client state is bounded to 256 entries, with at most 512 changes per page. Avatar parcel privacy and exclusion of attachments are applied before publication. No private object inventory or script sources are exported.

This version carries entity metadata alongside the compatibility scene. It does not replace physics, object movement packets, the renderer, inventory delivery or region crossings. The physics prototype and bundle validator are not advertised as active simulation/publishing features. Audio component vocabulary does not enable voice services.
