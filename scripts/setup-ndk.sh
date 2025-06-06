#!/usr/bin/env bash

set -euxo pipefail

NDK_VERSION=${NDK_VERSION:-"21.4.7075529"}
EXT=""
if [[ "$TRAVIS_OS_NAME" == "windows" ]]; then
    EXT=.bat
fi

SDKMANAGER=$ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager$EXT
echo y | $SDKMANAGER "ndk;$NDK_VERSION"
ln -sfn $ANDROID_HOME/ndk/$NDK_VERSION $ANDROID_HOME/ndk-bundle
