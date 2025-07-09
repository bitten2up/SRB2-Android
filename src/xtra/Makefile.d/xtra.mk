#
# Makefile for feature flags.
# XTRA Edition!
#

CURRENT_OPTS:= 
CURRENT_SOURCES:=

ifndef ANDROID
  opts+=-DTOUCHINPUTS -DNATIVESCREENRES # -DHAVE_WHANDLE
  ifneq ($(findstring -DHWRENDER, $(opts)),)
    opts+=-DHAVE_GLES -DHAVE_GLES2
  endif
  CURRENT_OPTS+=$(opts)
  #CURRENT_SOURCES:=$(sources)
else
  CURRENT_OPTS+=$(LOCAL_CFLAGS)
  #CURRENT_SOURCES:=$(LOCAL_SRC_FILES)
endif

ifndef ANDROID
LOCAL_PATH:=.
SRC_MAIN:=$(LOCAL_PATH)
SRC_HWR:=hardware
SRC_SDL:=sdl
SRC_APK:=android
SRC_XTRA:=xtra
endif

CURRENT_SOURCES+=$(SRC_MAIN)/w_handle.c
ifndef ANDROID

CURRENT_SOURCES+=\
  $(call List,$(LOCAL_PATH)/$(SRC_APK)/Sourcefile)\
  $(call List,$(LOCAL_PATH)/$(SRC_XTRA)/Sourcefile)\

endif

ifndef NOHW
  #ifeq (, $(findstring -DHWRENDER, $(CURRENT_OPTS)))
  #ifdef HAVE_GLES2
  ifneq ($(findstring -DHAVE_GLES2, $(CURRENT_OPTS)),)
    CURRENT_OPTS+=-DHAVE_GLES2
    CURRENT_SOURCES+=$(SRC_HWR)/r_gles/r_gles2.c $(SRC_SDL)/ogl_es_sdl.c
  #else ifdef HAVE_GLES
  #else ifeq (, $(findstring -DHAVE_GLES,$(CURRENT_OPTS)))
  else ifneq ($(findstring -DHAVE_GLES, $(CURRENT_OPTS)),)
    CURRENT_OPTS+=-DHAVE_GLES
    CURRENT_SOURCES+=$(SRC_HWR)/r_gles/r_gles1.c $(SRC_SDL)/ogl_es_sdl.c
  else
    CURRENT_SOURCES+=$(SRC_HWR)/r_opengl/r_opengl.c $(SRC_SDL)/ogl_sdl.c
  endif
endif

ifndef ANDROID
  opts+=$(CURRENT_OPTS)
  sources+=$(CURRENT_SOURCES)
else
  LOCAL_CFLAGS+=$(CURRENT_OPTS)
  LOCAL_SRC_FILES+=$(CURRENT_SOURCES)
endif
