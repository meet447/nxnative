# GitHub Actions helpers

The **NxNative - iPhone 15 IPA** workflow (`.github/workflows/iphone15-ipa.yml`) runs a **single** `scripts/build_ipa_from_nsp.py` pass on the checked-out fork.

`tests`, `lint-c`, and `lint-python` do **not** run on pushes to `main` in this fork (they compete for `xcode-27` with IPA builds). They still run on pull requests and via **Run workflow**.

## Slim IPA + S3 (default)

With **`slim_ipa`** on (default), you do **not** set a download URL by hand:

1. Workflow builds a slim IPA (RomFS left out) and bakes a generated public URL into `romfs.url`
2. Uploads `RomFS.img` to S3 at that key
3. You download the IPA artifact; the phone fetches game data from S3 on first launch

### Repo secrets

| Secret | Required | Purpose |
|--------|----------|---------|
| `S3_BUCKET` | yes (for slim) | Bucket name |
| `AWS_ACCESS_KEY_ID` | yes | Upload credentials |
| `AWS_SECRET_ACCESS_KEY` | yes | Upload credentials |
| `AWS_REGION` | recommended | e.g. `ap-south-1` (default `us-east-1`) |
| `S3_PREFIX` | optional | Key prefix (default `nxnative`) → `s3://bucket/nxnative/<titleId>/romfs.img` |
| `S3_PUBLIC_BASE_URL` | optional | CloudFront / custom base, e.g. `https://cdn.example.com` — object URL becomes `{base}/{prefix}/{titleId}/romfs.img` |

Without `S3_PUBLIC_BASE_URL`, the baked URL is:

`https://<bucket>.s3.<region>.amazonaws.com/<prefix>/<titleId>/romfs.img`

Make that object publicly readable (bucket policy or CloudFront). S3 supports HTTP Range (resume).

### Dispatch inputs

- **`slim_ipa`**: on → S3 slim path; off → full RomFS inside the IPA  
- **`romfs_url`**: advanced override (bake this URL, skip S3 upload)  
- **`game_url` / `keys_url`**: as before  

### Local (no S3 automation)

```bash
python3 scripts/build_ipa_from_nsp.py <NSP> -k prod.keys \
  --romfs separate --romfs-url 'https://…/romfs.img' --keep-extracted
```

Some community builder repos wrap IPA production in `nxnative_ci_guard.py`, which builds twice and hands RomFS between passes. That guard and the matching host or profile changes are **not** in `meet447/nxnative`, so this fork’s workflow does not call it.
