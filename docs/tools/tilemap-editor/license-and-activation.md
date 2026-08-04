# Tilemap Editor — License & Activation

**Level**: ⭐⭐ Intermediate

---

> **Quick Index**
> - [What the license unlocks](#what-the-license-unlocks)
> - [Activation](#activation)
> - [License key format](#license-key-format)
> - [Machine binding (fingerprint)](#machine-binding-fingerprint)
> - [License storage](#license-storage)
> - [Deactivation](#deactivation)
> - [Troubleshooting](#troubleshooting)

---

## What the license unlocks

**C++ export is the only feature gated behind a valid license.** Tilemap editing, saving, painting, and all other editor functions work without a license.

| Feature | Without license | With license |
|---------|:---:|:---:|
| Create / edit tilemaps | ✅ | ✅ |
| Import tilesets | ✅ | ✅ |
| Save projects | ✅ | ✅ |
| Animations, attributes, layers | ✅ | ✅ |
| **C++ export** | 🔒 | ✅ |

Attempting to export without a license shows an **"Upgrade Required"** dialog with two options: enter a license key, or continue without exporting.

---

## Activation

### Trial / activation dialog

On first launch, the Tool Suite shows a **"License Activation"** modal:

- **Enter a license key** and click **Activate License**.
- A **5-second countdown** gates the **Later** button — you must see the purchase link before dismissing.
- The **Activate** button is always available; only dismissal is restricted.

You can also access activation via:

- **License Info** button in the Tool Suite launcher footer
- The **"Enter License Key"** button on the "Upgrade Required" dialog

> **Note:** Inside the Tilemap Editor, the module owns its own toolbar, so the global app `Help → License Info` menu item is not reachable. Use the launcher footer button instead.

### License Info dialog

Opened from the launcher footer: displays the current license status, masked key, product version, and activation date. A **Deactivate** button clears the license from this machine.

---

## License key format

```
PR32-3X-{keyid}-{payload_base64url}-{signature_base64url}
```

| Segment | Description |
|---------|-------------|
| `PR32-3X-` | Fixed prefix — identifies v3 license keys |
| `keyid` | Integer identifying which signing key was used (currently: `1`) |
| `payload` | Base64url-encoded JSON payload (no padding) |
| `signature` | Ed25519 signature over the raw payload bytes (86 base64url chars) |

### Validation pipeline

1. **Format check** — regex validates the key structure
2. **Key lookup** — the keyid maps to an embedded Ed25519 public key
3. **Signature verification** — Ed25519 PureEdDSA over raw payload bytes
4. **Payload checks** — JSON payload must contain valid `v` (schema), `p` (product code `PR32TS`), and `ver` (product version)
5. **Version compatibility** — license `ver` major must match the running app version

> **Your public key is embedded in the app.** The private key that signs licenses never ships in any binary. Extracting the public key does not allow forging licenses.

### Masked display

In the License Info dialog and status bar, the key is shown partially masked:

```
PR32-3X-1-****-XXXXXXXX
```

Only the last 8 characters of the signature are visible — enough to match the key against your purchase email.

---

## Machine binding (fingerprint)

Licenses are bound to your machine to prevent casual sharing (not DRM — a courtesy measure).

### How the fingerprint is generated

```
SHA-256(hostname | OS | OS_major_version | architecture)
→ truncated to 16 hexadecimal characters
```

Example components: `DESKTOP-ABC | Windows | 10 | AMD64` → `a1b2c3d4e5f6a7b8`

### Change tolerance

| Rule | Behaviour |
|------|-----------|
| **Same machine** | Fingerprint matches → license valid |
| **One change** | E.g. OS upgrade, hostname rename → one-time tolerance, license stays valid for this session |
| **Two or more changes** | Crosses `MAX_CHANGES` threshold → license invalid, **re-activation required** |

Changes that trigger a fingerprint mismatch: OS version major change, hostname change, architecture change (e.g. migration to a new CPU).

> **Note:** The change counter is tracked in memory for the current session only — it resets every launch, and a tolerated mismatch does not rewrite the stored fingerprint. The stored fingerprint is replaced only when you re-activate the license. Re-activation does NOT consume a new license seat.

---

## License storage

### On-disk file

```
~/.pixelroot32/license.enc
```

**Format:** `ENC:` + Base64( `IV[16 bytes]` + AES-256-CBC ciphertext )

The ciphertext contains a JSON object with the license key, product version, activation timestamp, system fingerprint, and a SHA-256 checksum.

### Encryption key

Derived from machine-specific data: `SHA-256(MAC address | hostname | OS | architecture)`. This is NOT a secret — it is an anti-tamper measure, not DRM.

On **Linux**, the key is optionally cached in the desktop Secret Service (libsecret) when available, avoiding repeated derivation.

### Tamper detection

The stored JSON includes a SHA-256 checksum over the license fields. Any manual modification of the file invalidates the checksum and the license is rejected. The app re-validates the file on every launch.

---

## Deactivation

1. Open **Help → License Info** (or the launcher footer **License Info** button)
2. Click **Deactivate License**
3. The `license.enc` file is removed from disk

After deactivation, the Tool Suite returns to the trial state (only C++ export is gated). You can reactivate with the same key on the same machine, or transfer the license to a different machine.

---

## Troubleshooting

### "Invalid license key format"

- The key must be pasted exactly as received — no extra spaces, line breaks, or quote characters.
- Verify the key starts with `PR32-3X-` and contains four segments separated by `-`.

### "License verification failed"

- The key may be corrupted or was edited. Request a reissue from [pixelroot32.com](https://pixelroot32.com).

### "This license is for a different product"

- The key was issued for a different PixelRoot32 product. Verify you purchased a **Tool Suite** license.

### "This license is for version X, but you have version Y"

**Major version mismatch.** You have either:

- Upgraded the Tool Suite past the license's covered version → **renew** your license.
- Downgraded the Tool Suite to a version older than the license was issued for → **update** the Tool Suite.

The major version is the first number: `1.0` and `1.9` are compatible (both major `1`); `1.0` and `2.0` are NOT.

### "Unknown license key id"

The key's signing key is not recognized. This happens if the key was issued with a newer key pair that hasn't been embedded in your Tool Suite build yet. **Update** the Tool Suite to the latest version.

### "Hardware changed" / fingerprint mismatch

You changed your machine significantly (OS major upgrade, new hardware). The license tolerates **one** such change. If you've already exceeded the limit, simply **reactivate with your existing key**: the old fingerprint is replaced and the counter resets.

---

## See also

- [Quick Start](/tools/tilemap-editor/quick-start) — includes export flow
- [Advanced Guide](/tools/tilemap-editor/advanced-guide) — license system in the export pipeline
- [Technical Reference](/tools/tilemap-editor/technical-reference) — C++ export format details
- [Tools overview](/tools/)
