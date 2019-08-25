#!/bin/sh
# this script can be used to download official Microsoft ESD files for tentoolkit

# Microsoft product URL
PRODUCT_URL="https://go.microsoft.com/fwlink/?LinkId=841361"

# make sure all dependencies are installed
DEPS=("curl" "7z")
for i in ${DEPS[@]}; do
	if ! [ -x "$(command -v curl)" ]; then
		echo "Please install $i" >&2
		exit 1
	fi
done

# basic error handling
set -e

# download and uncompress products XML
curl -L $PRODUCT_URL > products.cab
7z e products.cab products.xml
rm products.cab

# filter URLs
TMP=/tmp/links_$(date +%N)

# create links file
sort -u products.xml | tr -d ' ' | tr -d '\r' \
	| sed -n 's/<FilePath>\([^<]\+\)<\/FilePath>/\1/p' > $TMP
rm products.xml

# edit links file
vi $TMP

# download ESDs
for i in `cat $TMP`; do
	curl -L "$i" > `basename $i`
done
rm $TMP
