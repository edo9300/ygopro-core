APP_ABI := armeabi-v7a arm64-v8a x86
APP_PLATFORM := android-16
APP_ALLOW_MISSING_DEPS=true
APP_STL := c++_static
APP_CPPFLAGS := -Wno-error=format-security -std=c++14 -fpermissive -fvisibility=hidden -DOCGCORE_EXPORT_FUNCTIONS
APP_OPTIM := release
