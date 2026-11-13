# ForMe Real World Simulation: client packages RWS-001 / RWS-002

The new native entity cache uses the authenticated `ForMeWorld` capability and the shared [protocol](../protocol/README.md). It preserves existing viewer objects and their UUIDs. Region/session changes clear the cache; responses from previous sessions, cursor gaps, stale revisions and oversized pages are rejected.

Creators can open **ForMe.World → Object physics**, select an editable object and select static/dynamic body mode, standard/convex collision shape, material presets, density, friction, restitution, gravity and collision flag through the server. The server rechecks its own role, creator grant, object/land permissions and revision. Region and backend limits are enforced. A direct selected-object read avoids depending on the nearest-200 interest cache. Experimental soft-body physics is not active. A revoked creator grant also closes the general creation/upload UI at the next badge refresh.

Canonical project status, specification, audit, Windows server instructions and remaining work are tracked in the [server repository](https://github.com/amwbru-alt/SERVER/blob/fix/server-audit-20261113/doc/real-world/STATUS.md). In-world acceptance A–H, independent rendering, the full bundle pipeline and the financial cutover are not complete in this package. No client installer is generated automatically by this change.

Run the standalone production replica tests:

```sh
g++ -std=c++20 -Wall -Wextra -Werror -pedantic -Iindra/llcommon tests/forme/world_replica_test.cpp -o /tmp/world-replica-test
/tmp/world-replica-test
```

RWS-002 follows roadmap stages 1–3. Shared component and rigid-profile JSON schemas and C# serializer vectors are stored under `protocol`. The financial cutover is deferred separately. Next work follows the roadmap: preserve remaining user scenarios through adapters, finish common contacts/constraints/interaction events, then deformable objects.

```sh
g++ -std=c++20 -Wall -Wextra -Werror -pedantic -Iindra/llcommon tests/forme/world_physics_test.cpp -o /tmp/world-physics-test
/tmp/world-physics-test
```
