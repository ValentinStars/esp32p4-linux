#!/usr/bin/env python3
import sys
import os

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IMAGE_PATH = os.path.join(REPO_ROOT, "kernel", "Image")
CPIO_GZ_PATH = os.path.join(REPO_ROOT, "kernel", "rootfs.cpio.gz")

INITRAMFS_OFFSET = 1053252
INITRAMFS_MAX_LEN = 352748

def main():
    if not os.path.exists(IMAGE_PATH) or not os.path.exists(CPIO_GZ_PATH):
        print(f"Error: Missing {IMAGE_PATH} or {CPIO_GZ_PATH}")
        sys.exit(1)

    with open(IMAGE_PATH, "rb") as f:
        img = bytearray(f.read())

    with open(CPIO_GZ_PATH, "rb") as f:
        gz_data = f.read()

    if len(gz_data) > INITRAMFS_MAX_LEN:
        print(f"Error: rootfs.cpio.gz ({len(gz_data)} bytes) exceeds allocated slot ({INITRAMFS_MAX_LEN} bytes)")
        sys.exit(1)

    img[INITRAMFS_OFFSET : INITRAMFS_OFFSET + INITRAMFS_MAX_LEN] = b"\x00" * INITRAMFS_MAX_LEN
    img[INITRAMFS_OFFSET : INITRAMFS_OFFSET + len(gz_data)] = gz_data

    with open(IMAGE_PATH, "wb") as f:
        f.write(img)

    print(f"✓ Injected rootfs.cpio.gz ({len(gz_data)} bytes) into kernel/Image at offset {INITRAMFS_OFFSET}")

if __name__ == "__main__":
    main()
