# GitHub Actions helpers

The **NxNative - iPhone 15 IPA** workflow (`.github/workflows/iphone15-ipa.yml`) runs a **single** `scripts/build_ipa_from_nsp.py` pass on the checked-out fork.

Some community builder repos wrap IPA production in `nxnative_ci_guard.py`, which builds twice and hands RomFS between passes. That guard and the matching host or profile changes are **not** in `meet447/nxnative`, so this fork’s workflow does not call it.

To add a two-pass guard later, copy the script from the upstream builder repo and wire it only after the RomFS handoff patches exist in-tree.