#!/usr/bin/env bash

set -euxo pipefail

NDK_VERSION=${NDK_VERSION:-"21.4.7075529"}

SDKMANAGER=$ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager
echo y | $SDKMANAGER "ndk;$NDK_VERSION"
ln -sfn $ANDROID_HOME/ndk/$NDK_VERSION $ANDROID_HOME/ndk-bundle

if [[ "$OS_NAME" == "osx" ]]; then
	$ANDROID_HOME/ndk-bundle/ndk-build -j3
fi
if [[ "$OS_NAME" == "linux" ]]; then
	$ANDROID_HOME/ndk-bundle/ndk-build -j2
fi
