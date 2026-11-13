# ForMe.World world protocol v1

`forme-world-v1.schema.json` is copied unchanged to the server and viewer repositories. The active capability is `ForMeWorld`; its random URL is registered before avatar presence creation, bound to the authenticated login session, and usable only by a live root presence.

POST requests:

- `{"action":"info"}` returns the active backend and implemented feature flags.
- `{"action":"poll","epoch":"","after":"0"}` starts a bounded interest subscription. On later polls, send the returned epoch and decimal cursor. A repeated unacknowledged cursor replays the previous page. A new epoch/reset replaces the entire local projection. A tombstone also means leaving interest, not necessarily deleting the actual world object. Clients reject cursor gaps, stale revisions and partial invalid pages.
- `{"action":"material","entityId":"UUID","expectedRevision":"N","material":{...}}` updates only the physical properties implemented by the active backend, after server role, additional creator grant, object and land checks. It does not advance the polling cursor. Read the next replica revision before another edit.

Region coordinates are metres, Z-up; the existing map grid origin is `(gridX * 256, gridY * 256, 0)`. All revisions, authority epochs and cursors are decimal nonnegative int64 strings. Session/node IDs are UUIDs. The current server projection is limited to 200 nearest entities; client state is bounded to 256 entries, with at most 512 changes per page. Avatar parcel privacy and exclusion of attachments are applied before publication. No private object inventory or script sources are exported.

This version carries entity metadata alongside the compatibility scene. It does not replace physics, object movement packets, the renderer, inventory delivery or region crossings. The physics prototype and bundle validator are not advertised as active simulation/publishing features. Audio component vocabulary does not enable voice services.

## Roadmap package RWS-002

The typed data contracts for all 20 standard components are in `forme-world-components-v1.schema.json`; `fixtures/components-v1.json` is emitted by the production C# serializer. Structural JSON rules are shared with the viewer. Cross-field ownership, hierarchy cycles, quaternion normalization and active backend support are also checked in server code. Declaring skeleton, cloth or fluid data does not start a corresponding solver.

Additional POST operations:

- `{"action":"world"}` reads persistent region identities, geometry, writer epochs and current journal leases. The `online` flag describes a live journal writer, not proof of physics handoff or avatar visibility.
- `{"action":"physics_read","entityId":"ROOT_UUID"}` reads the selected root object directly. Selection does not depend on inclusion in the nearest-200 interest cache. Current region visibility is checked.
- `{"action":"physics_profile","entityId":"ROOT_UUID","expectedRevision":"N","profile":{...}}` applies `forme-rigid-profile-v1.schema.json`: static/dynamic mode, primitive/convex collision representation, density, friction, restitution, gravity modifier and collision flag. Dynamic bodies require explicit backend support and region size/linkset limits. Backend rejection or journal failure restores preceding per-part properties. A scripted volume detector retains its collision authority; temporary objects are excluded from persistent profile editing.

`rigid_body.massPending=true` explicitly represents a requested dynamic body whose mass is not yet available from the engine. A small invented mass is not substituted. A malformed observed entity is excluded from replication until corrected; the rest of the region continues capturing. The existing engine remains the sole simulation authority.
