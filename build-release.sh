#!/bin/bash

# zipped up binaries will be written to ./artifacts
mkdir -p artifacts

docker build -t fab-agon-emulator-buildenv -f ./dist_scripts/Dockerfile .

if [ $? -ne 0 ]; then
	echo "Failed to build docker image!"
	exit -1
fi

echo Building binaries...

docker run -i -v ./artifacts:/build/artifacts fab-agon-emulator-buildenv  ./dist_scripts/make-dist-linux.sh
docker run -i -v ./artifacts:/build/artifacts fab-agon-emulator-buildenv  ./dist_scripts/make-dist-windows.sh
