# GitHub Actions helpers

The **NxNative - iPhone 15 IPA** workflow (`.github/workflows/iphone15-ipa.yml`) runs a **single** `scripts/build_ipa_from_nsp.py` pass on the checked-out fork.

`tests`, `lint-c`, and `lint-python` do **not** run on pushes to `main` in this fork (they compete for `xcode-27` with IPA builds). They still run on pull requests and via **Run workflow**.

## Slim IPA + VPS RomFS download

Pass **`romfs_url`** (HTTP(S) URL of `RomFS.img` on your VPS) when dispatching the workflow, or locally:

```bash
python3 scripts/build_ipa_from_nsp.py <NSP> -k prod.keys --romfs separate --romfs-url 'https://your.vps/scarlet/romfs.img' --keep-extracted
```

The IPA omits the game data (~6+ GB for Scarlet), bakes `romfs.url` + `romfs.id`, and on first launch downloads `Documents/romfs.img` (resume supported). Upload the build’s `…/extract/<ver>/RomFS.img` to that exact URL (byte-identical; the app checks size against `romfs.id`).

Some community builder repos wrap IPA production in `nxnative_ci_guard.py`, which builds twice and hands RomFS between passes. That guard and the matching host or profile changes are **not** in `meet447/nxnative`, so this fork’s workflow does not call it.

To add a two-pass guard later, copy the script from the upstream builder repo and wire it only after the RomFS handoff patches exist in-tree.