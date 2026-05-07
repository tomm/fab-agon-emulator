#!/bin/bash
export APP_VERSION=`cargo tree --depth 0 | awk '{print $2;}'`
export ARCH=`uname -m`

flatpak-builder --force-clean --user --install-deps-from=flathub \
  --repo=repo --install builddir dist_scripts/io.github.tomm.fab_agon_emulator.yml

flatpak build-bundle repo fab-agon-emulator-$APP_VERSION-$ARCH.flatpak io.github.tomm.fab_agon_emulator --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo
