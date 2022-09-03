#!/usr/bin/env bash

set -euxo pipefail

mkdir -p $DEPLOY_DIR
if [[ "$TRAVIS_OS_NAME" == "windows" ]]; then
	mv bin/$BUILD_CONFIG/ocgcore.dll $DEPLOY_DIR
	powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Compress-Archive -Path $DEPLOY_DIR -DestinationPath ocgcore-$TRAVIS_OS_NAME"
elif [[ "$TRAVIS_OS_NAME" == "android" ]]; then
	ARCH=("armeabi-v7a" "arm64-v8a" "x86" "x86_64" )
	OUTPUT=("libocgcorev7.so" "libocgcorev8.so" "libocgcorex86.so" "libocgcorex64.so")
	for i in {0..3}; do
		CORE="libs/${ARCH[i]}/libocgcore.so"
		if [[ -f "$CORE" ]]; then
			mv $CORE "$DEPLOY_DIR/${OUTPUT[i]}"
		fi
	done
	tar -zcf ocgcore-$TRAVIS_OS_NAME.tar.gz $DEPLOY_DIR
else
	if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
		mv bin/$BUILD_CONFIG/libocgcore.dylib $DEPLOY_DIR
	else
		mv bin/$BUILD_CONFIG/libocgcore.so $DEPLOY_DIR
	fi
	tar -zcf ocgcore-$TRAVIS_OS_NAME.tar.gz $DEPLOY_DIR
fi
