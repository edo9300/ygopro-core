# File: Android.mk
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := ocgcore
LOCAL_MODULE_FILENAME := libocgcore
LOCAL_SRC_FILES := card.cpp \
				duel.cpp \
				effect.cpp \
				field.cpp \
				group.cpp \
				interpreter.cpp \
				libcard.cpp \
				libdebug.cpp \
				libduel.cpp \
				libeffect.cpp \
				libgroup.cpp \
				ocgapi.cpp \
				operations.cpp \
				playerop.cpp \
				processor.cpp \
				processor_visit.cpp \
				scriptlib.cpp

LOCAL_CFLAGS   := -pedantic -Wextra -fvisibility=hidden -DOCGCORE_EXPORT_FUNCTIONS -DNDEBUG
LOCAL_CPPFLAGS := -fno-exceptions -fno-rtti
ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
LOCAL_LDFLAGS += "-Wl,-z,max-page-size=16384"
endif
LOCAL_STATIC_LIBRARIES += liblua

include $(BUILD_SHARED_LIBRARY)

$(call import-add-path,$(LOCAL_PATH))
$(call import-module,lua)

