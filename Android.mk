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

LOCAL_CFLAGS    :=  -pedantic -Wextra -fvisibility=hidden -DOCGCORE_EXPORT_FUNCTIONS
LOCAL_CPPFLAGS := -fexceptions -fno-rtti
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../lua/include
LOCAL_STATIC_LIBRARIES += liblua5.3

include $(BUILD_SHARED_LIBRARY)

$(call import-add-path,$(LOCAL_PATH)/..)
$(call import-module,lua)

