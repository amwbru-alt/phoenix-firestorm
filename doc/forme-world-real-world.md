# ForMe Real World Simulation: client package RWS-001

The new native entity cache uses the authenticated `ForMeWorld` capability and the shared [protocol](../protocol/README.md). It preserves existing viewer objects and their UUIDs. Region/session changes clear the cache; responses from previous sessions, cursor gaps, stale revisions and oversized pages are rejected.

Creators can open **ForMe.World → Object physics**, select an editable object and adjust density, friction and restitution through the server. The server rechecks its own role, creator grant, object/land permissions and revision. Other material properties and experimental soft-body physics are not active. A revoked creator grant also closes the general creation/upload UI at the next badge refresh.

Canonical project status, specification, audit, Windows server instructions and remaining work are tracked in the [server repository](https://github.com/amwbru-alt/SERVER/blob/fix/server-audit-20261113/doc/real-world/STATUS.md). In-world acceptance A–H, independent rendering, the full bundle pipeline and the financial cutover are not complete in this package. No client installer is generated automatically by this change.

Run the standalone production replica tests:

```sh
g++ -std=c++20 -Wall -Wextra -Werror -pedantic -Iindra/llcommon tests/forme/world_replica_test.cpp -o /tmp/world-replica-test
/tmp/world-replica-test
```
