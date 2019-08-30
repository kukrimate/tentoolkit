#!/bin/sh

# basic error handling
set -e

# we need cdrkit
if ! [ -x "$(command -v mkisofs)" ]; then
	echo "ERROR: mkisofs not found, please install cdrkit" >&2
	exit 1
fi

# check parameters
if [ "$#" -lt 2 ]; then
	echo "Usage: buildiso <media_path> <iso_path>" >&2
	exit 1
fi

# check media directory
if [ ! -d "$1" ]; then
	echo "ERROR: Media directory does not exist!" >&2
	exit 1
fi

MKISOFS_FLAGS="-quiet -UDF"

# check boot files
if [ -f "$1/boot/etfsboot.com" ]; then
	MKISOFS_FLAGS="$MKISOFS_FLAGS -no-emul-boot -boot-load-size 8 -b boot/etfsboot.com"
else
	echo "WARNING: etfsboot.com not found, image will *NOT* be BIOS bootable!!" >&2
fi
if [ -f "$1/efi/microsoft/boot/efisys.bin" ]; then
	MKISOFS_FLAGS="$MKISOFS_FLAGS -eltorito-alt-boot -eltorito-platform efi \
		-no-emul-boot -b efi/microsoft/boot/efisys.bin"
else
	echo "WARNING: efisys.bin not found, image will *NOT* be (U)EFI bootable!!" >&2
fi

# build the image
mkisofs $MKISOFS_FLAGS -o "$2" "$1"
