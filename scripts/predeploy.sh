#!/usr/bin/env bash

set -euxo pipefail
TARGET_OS=${TARGET_OS:-$TRAVIS_OS_NAME}

mkdir -p $DEPLOY_DIR
if [[ "$TARGET_OS" == "windows" ]]; then
	shopt -s globstar
	mv bin/**/ocgcore.dll $DEPLOY_DIR
	if [[ "${STRIP:-0}" == "1" ]]; then
		strip $DEPLOY_DIR/ocgcore.dll
	fi
elif [[ "$TARGET_OS" == "android" ]]; then
	ARCH=("armeabi-v7a" "arm64-v8a" "x86" "x86_64" "armeabi" "mips" "mips64" "riscv64")
	OUTPUT=("libocgcorev7.so" "libocgcorev8.so" "libocgcorex86.so" "libocgcorex64.so" "libocgcoreeabi.so" "libocgcoremips.so" "libocgcoremips64.so" "libocgcoreriscv.so")
	for i in {0..7}; do
		CORE="libs/${ARCH[i]}/libocgcore.so"
		if [[ -f "$CORE" ]]; then
			mv $CORE "$DEPLOY_DIR/${OUTPUT[i]}"
		fi
	done
else
	if [[ "$TARGET_OS" == "macosx" ]]; then
		mv bin/$BUILD_CONFIG/libocgcore.dylib $DEPLOY_DIR
	else
		mv bin/$BUILD_CONFIG/libocgcore.so $DEPLOY_DIR
	fi
fi
