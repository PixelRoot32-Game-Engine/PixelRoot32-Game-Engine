# Keeps the upload from writing into the `nvs` partition.
#
# The Arduino-ESP32 build script always adds boot_app0.bin (8 KB, an initial
# `otadata` record) at 0xE000, the otadata offset of the stock OTA tables. In
# the DOOM table (partitions_wad.csv) there is no otadata partition and
# 0xE000..0xF000 is the LAST 4 KB page of `nvs` (0x9000..0xF000): flashing
# boot_app0 there erases that NVS page and writes an otadata word into its
# header, which can destroy whatever NVS entries lived on it (DOOM's doom/cfg
# included). A factory-only table needs no otadata, so this script drops that
# image from the upload. Every other image (bootloader, partition table, app)
# is flashed as usual.
#
# Runs as a `post:` script, after the platform has assembled UPLOADERFLAGS.

Import("env")  # noqa: F821  (SCons global)

OTADATA_OFFSET = 0xE000


def _is_otadata(offset):
    try:
        return int(str(offset), 0) == OTADATA_OFFSET
    except ValueError:
        return False


images = env.get("FLASH_EXTRA_IMAGES", [])  # noqa: F821
kept = [img for img in images if not _is_otadata(img[0])]
env.Replace(FLASH_EXTRA_IMAGES=kept)  # noqa: F821

flags = list(env.get("UPLOADERFLAGS", []))  # noqa: F821
filtered = []
skip_next = False
for flag in flags:
    if skip_next:
        skip_next = False
        continue
    if isinstance(flag, str) and _is_otadata(flag):
        skip_next = True  # drop the offset and the image path after it
        continue
    filtered.append(flag)
env.Replace(UPLOADERFLAGS=filtered)  # noqa: F821

print("keep_nvs.py: upload images %s (0xE000 boot_app0 removed: %s)" % (
    [str(img[0]) for img in kept], len(flags) != len(filtered)))
