# ForMe.World macOS build in Actions

Use the registered **Build** workflow, ID `395853243`, with the source branch
`forme-single-grid-20261025`. The inherited `build_sl_mac_only.yml` filename was
not resolvable through the fork's Actions API; repeating authentication does not
fix that lookup.

Manual `workflow_dispatch` runs select only the `forme-macos` job. The inherited
setup/build/sign/release jobs are skipped because their setup job excludes manual
runs. No master change or PR merge is needed to select the branch's workflow
implementation through this existing registered workflow ID.

The job uses the standard `macos-15-intel` runner, Xcode 16.4 and the repository's
`ReleaseFS_open` configuration. The project builds arm64 and x86_64 together.
KDU, FMOD, Havok, Bugsplat and Discord integration are disabled; OpenAL and the
fixed server policy are retained. Public dependency downloads may still fail
independently and will be visible in the configure log.

There are no rclone, deployment, webhook, signing-certificate or notarization
steps. Only the standard read-only GitHub token is used for public packages.
The app receives a local ad-hoc signature, not an Apple Developer signature or
notarization. This is a test application artifact, not a notarized installer.

## Start from a terminal

After `gh auth login` has succeeded:

```bash
FORME_REPO="amwbru-alt/phoenix-firestorm"
FORME_BRANCH="forme-single-grid-20261025"

gh workflow run 395853243 --repo "$FORME_REPO" --ref "$FORME_BRANCH"
gh run list --repo "$FORME_REPO" --workflow 395853243 \
  --branch "$FORME_BRANCH" --event workflow_dispatch --limit 5
```

Use the ID of the new run; do not select an older PR run or the policy-test run.

```bash
FORME_RUN_ID=1234567890  # replace with the new run ID

gh run watch "$FORME_RUN_ID" --repo "$FORME_REPO" --exit-status &&
gh run download "$FORME_RUN_ID" --repo "$FORME_REPO" \
  --name ForMe.World-macOS --dir "$HOME/Downloads/ForMeWorld-mac"
```

The `ForMe.World-macOS` artifact contains `ForMe.World-macOS-universal.zip` with
`ForMe.World.app`. Keep the inner ZIP intact until extracting on macOS to preserve
bundle permissions and symbolic links. Successful bundle archiving checks the
main executable and resources, then verifies the ad-hoc signature.

If the run fails:

```bash
gh run view "$FORME_RUN_ID" --repo "$FORME_REPO" --log-failed
gh run download "$FORME_RUN_ID" --repo "$FORME_REPO" \
  --name ForMe.World-macOS-logs --dir "$HOME/Downloads/ForMeWorld-mac-logs"
```

The app artifact is uploaded only on success. Configure/build logs are uploaded
with `always()` when present. Retention is 14 days. A successful full macOS build
and in-world functional validation have not yet been demonstrated.
