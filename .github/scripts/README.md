# GitHub Actions helpers

The **NxNative - iPhone 15 IPA** workflow (`.github/workflows/iphone15-ipa.yml`) runs a **single** `scripts/build_ipa_from_nsp.py` pass on the checked-out fork.

`tests`, `lint-c`, and `lint-python` do **not** run on pushes to `main` in this fork (they compete for `xcode-27` with IPA builds). They still run on pull requests and via **Run workflow**.

## Slim IPA + Lightsail bucket (default)

With **`slim_ipa`** on (default), you do **not** set a download URL by hand:

1. Workflow builds a slim IPA (RomFS left out) and bakes a generated public URL into `romfs.url`
2. Uploads `RomFS.img` to your **Lightsail object storage** bucket (S3-compatible API)
3. You download the IPA artifact; the phone fetches game data from that URL on first launch

### Lightsail setup

1. Lightsail → **Storage** → create a bucket (note its **region**, e.g. `ap-south-1`)
2. Bucket permissions: allow objects to be made public (or set Individual objects → Public)
3. **Access keys** on that bucket page → create → copy access key ID + secret

### Repo secrets

| Secret | Required | Purpose |
|--------|----------|---------|
| `S3_BUCKET` | yes | Lightsail bucket name |
| `AWS_ACCESS_KEY_ID` | yes | Lightsail bucket access key |
| `AWS_SECRET_ACCESS_KEY` | yes | Lightsail bucket secret |
| `AWS_REGION` | yes | Same region as the bucket (e.g. `ap-south-1`) |
| `S3_ENDPOINT` | optional | Default `https://s3.<region>.amazonaws.com` (correct for Lightsail) |
| `S3_PREFIX` | optional | Key prefix (default `nxnative`) → `…/nxnative/<titleId>/romfs.img` |
| `S3_PUBLIC_BASE_URL` | optional | CDN / custom base if you front the bucket |

Baked public URL (unless `S3_PUBLIC_BASE_URL` is set):

`https://<bucket>.s3.<region>.amazonaws.com/<prefix>/<titleId>/romfs.img`

Upload uses `aws s3 cp --endpoint-url … --acl public-read` so the object is downloadable without signed URLs. Range requests (resume) work on Lightsail.

### Dispatch inputs

- **`slim_ipa`**: on → Lightsail/S3 slim path; off → full RomFS inside the IPA  
- **`romfs_url`**: advanced override (bake this URL, skip upload)  
- **`game_url` / `keys_url`**: as before  

### Local (no upload automation)

```bash
python3 scripts/build_ipa_from_nsp.py <NSP> -k prod.keys \
  --romfs separate --romfs-url 'https://…/romfs.img' --keep-extracted
```

Some community builder repos wrap IPA production in `nxnative_ci_guard.py`, which builds twice and hands RomFS between passes. That guard and the matching host or profile changes are **not** in `meet447/nxnative`, so this fork’s workflow does not call it.
