LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ANDROID := 1

LOCAL_MODULE := main

# Paths

SRB2_PATH := ../../../..

SRC_JNI := .
SRC_MAIN := $(SRB2_PATH)/src

SRC_HWR := $(SRC_MAIN)/hardware/
SRC_SDL := $(SRC_MAIN)/sdl/

SRC_APK := $(SRC_MAIN)/android/
SRC_XTRA := $(SRC_MAIN)/xtra/

ifeq ($(OS),Windows_NT)
WINDOWSHELL=1
endif

MAKE_DIR := $(LOCAL_PATH)/$(SRC_MAIN)/Makefile.d
XTRA_MAKE_DIR := $(LOCAL_PATH)/$(SRC_XTRA)/Makefile.d

# Compile flags
# remove nativescreenres eventually, we have any-res now

#-DHWRENDER -DHAVE_GLES -DHAVE_GLES2
HAVE_GLES:=1
HAVE_GLES2:=1
LOCAL_CFLAGS += -DUNIXCOMMON -DLINUX \
				-DHAVE_SDL -DHAVE_MIXER -DHAVE_MIXERX -DHAVE_LIBGME \
				-DHWRENDER -DHAVE_GLES2 \
				-DTOUCHINPUTS -DNATIVESCREENRES -DDIRECTFULLSCREEN \
				-DHAVE_ZLIB -DHAVE_PNG -DHAVE_CURL \
				-DHAVE_WHANDLE -DHAVE_THREADS -DLOGCAT -DCOMPVERSION \
				-DNONX86 -DNOASM -DNOMUMBLE

# Includes

include $(MAKE_DIR)/platform.mk
include $(MAKE_DIR)/util.mk

# Source files

LOCAL_SRC_FILES := $(call List,$(LOCAL_PATH)/$(SRC_JNI)/Sourcefile)
LOCAL_SRC_FILES += $(call List,$(LOCAL_PATH)/$(SRC_APK)/Sourcefile)
LOCAL_SRC_FILES += $(call List,$(LOCAL_PATH)/$(SRC_XTRA)/Sourcefile)
LOCAL_SRC_FILES += $(call List,$(LOCAL_PATH)/$(SRC_MAIN)/Sourcefile)
LOCAL_SRC_FILES += $(call List,$(LOCAL_PATH)/$(SRC_MAIN)/blua/Sourcefile)
LOCAL_SRC_FILES += $(call List,$(LOCAL_PATH)/$(SRC_MAIN)/netcode/Sourcefile)
LOCAL_SRC_FILES += $(call List,$(LOCAL_PATH)/$(SRC_HWR)/Sourcefile)
LOCAL_SRC_FILES += $(call List,$(LOCAL_PATH)/$(SRC_SDL)/Sourcefile)

include $(XTRA_MAKE_DIR)/xtra.mk

LOCAL_SRC_FILES += $(SRC_SDL)/SDL_main/SDL_android_main.c $(SRC_SDL)/mixer_sound.c $(SRC_SDL)/i_threads.c
LOCAL_SRC_FILES += $(SRC_MAIN)/comptime.c $(SRC_MAIN)/md5.c

# Libraries

LOCAL_SHARED_LIBRARIES := SDL2 hidapi \
	SDL2_mixer libmpg123 \
	libpng libgme
LOCAL_STATIC_LIBRARIES := libcurl
LOCAL_LDLIBS := -lGLESv2 -lEGL -llog -lz

include $(BUILD_SHARED_LIBRARY)
