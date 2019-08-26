#!/usr/bin/env bash

set -euxo pipefail

mkdir -p $DEPLOY_DIR
cp LICENSE README.md $DEPLOY_DIR
if [[ "$TRAVIS_OS_NAME" == "windows" ]]; then
	mv build/$BUILD_CONFIG/ocgcore.dll build/$BUILD_CONFIG/ocgcore.lib build/$BUILD_CONFIG/ocgcore_static.lib $DEPLOY_DIR
	powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "Compress-Archive -Path $DEPLOY_DIR -DestinationPath ocgcore-$TRAVIS_OS_NAME"
else
	mv build/libocgcore_static.a $DEPLOY_DIR
	if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
		mv build/libocgcore.dylib $DEPLOY_DIR
	else
		mv build/libocgcore.so $DEPLOY_DIR
	fi
	tar -zcf ocgcore-$TRAVIS_OS_NAME.tar.gz $DEPLOY_DIR
fi
