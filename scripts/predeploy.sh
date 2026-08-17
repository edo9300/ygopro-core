#!/usr/bin/env bash

set -euxo pipefail
TARGET_OS=${TARGET_OS:-$TRAVIS_OS_NAME}
STRIP_BIN="${STRIP_BIN:-strip}"
OBJCOPY_BIN="${OBJCOPY_BIN:-objcopy}"

function strip_binary {
    if [[ "${STRIP:-1}" == "1" ]]; then
		$OBJCOPY_BIN --only-keep-debug $DEPLOY_DIR/$1 $DEPLOY_DIR/$1.debug
        $STRIP_BIN $DEPLOY_DIR/$1
		tar -zcvf $DEPLOY_DIR/$1.debug.tgz -C $DEPLOY_DIR $1.debug
		rm $DEPLOY_DIR/$1.debug
    fi
}

mkdir -p $DEPLOY_DIR
if [[ "$TARGET_OS" == "windows" ]]; then
	shopt -s globstar
	mv bin/**/ocgcore.dll $DEPLOY_DIR
	strip_binary ocgcore.dll
elif [[ "$TARGET_OS" == "android" ]]; then
	ARCH=("armeabi-v7a" "arm64-v8a" "x86" "x86_64" "armeabi" "mips" "mips64" "riscv64")
	OUTPUT=("libocgcorev7.so" "libocgcorev8.so" "libocgcorex86.so" "libocgcorex64.so" "libocgcoreeabi.so" "libocgcoremips.so" "libocgcoremips64.so" "libocgcoreriscv.so")
	for i in {0..7}; do
		CORE="libs/${ARCH[i]}/libocgcore.so"
		if [[ -f "$CORE" ]]; then
			mv $CORE "$DEPLOY_DIR/${OUTPUT[i]}"
			strip_binary "${OUTPUT[i]}"
		fi
	done
else
	if [[ "$TARGET_OS" == "macosx" ]]; then
		mv bin/$BUILD_CONFIG/libocgcore.dylib $DEPLOY_DIR
		strip_binary libocgcore.dylib
	else
		mv bin/$BUILD_CONFIG/libocgcore.so $DEPLOY_DIR
		strip_binary libocgcore.so
	fi
fi
