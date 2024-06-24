LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := lua
EXCLUDED_LUA_FILES := $(LOCAL_PATH)/src/lua.c $(LOCAL_PATH)/src/luac.c $(LOCAL_PATH)/src/ltests.h $(LOCAL_PATH)/src/ltests.c $(LOCAL_PATH)/src/onelua.c
FILE_LIST := $(filter-out $(EXCLUDED_LUA_FILES), $(wildcard $(LOCAL_PATH)/src/*.c))
LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)

LOCAL_CPPFLAGS := -fexceptions -fno-rtti -D"lua_getlocaledecpoint()='.'" -Wno-deprecated
LOCAL_EXPORT_CFLAGS := -D"lua_getlocaledecpoint()='.'"
LOCAL_CPP_EXTENSION := .c
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/src
LOCAL_THIN_ARCHIVE := true

include $(BUILD_STATIC_LIBRARY)
