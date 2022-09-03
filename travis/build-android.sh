#!/usr/bin/env bash

set -euxo pipefail

if [[ "$OS_NAME" == "osx" ]]; then
	ANDROID_HOME=$HOME/Library/Android/sdk
	SDKMANAGER=$ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager
	echo y | $SDKMANAGER "ndk;21.4.7075529"
	ln -sfn $ANDROID_HOME/ndk/21.4.7075529 $ANDROID_HOME/ndk-bundle
	$ANDROID_HOME/ndk-bundle/ndk-build -j3
fi
if [[ "$OS_NAME" == "linux" ]]; then
	ANDROID_ROOT=/usr/local/lib/android
	ANDROID_SDK_ROOT=${ANDROID_ROOT}/sdk
	SDKMANAGER=${ANDROID_SDK_ROOT}/cmdline-tools/latest/bin/sdkmanager
	echo "y" | $SDKMANAGER "ndk;21.4.7075529"
	ANDROID_NDK_ROOT=${ANDROID_SDK_ROOT}/ndk-bundle
	ln -sfn $ANDROID_SDK_ROOT/ndk/21.4.7075529 $ANDROID_NDK_ROOT
	$ANDROID_HOME/ndk-bundle/ndk-build -j2
fi
