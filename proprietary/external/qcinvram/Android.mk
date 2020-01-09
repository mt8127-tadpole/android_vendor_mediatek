LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES:= \
    $(MTK_PATH_SOURCE)/external/nvram/libnvram \
    $(MTK_PATH_SOURCE)/external/nvram/libfile_op \
    $(TOP)/mediatek/custom/$(MTK_PROJECT)/cgen/inc \
    $(TOP)/mediatek/custom/$(MTK_PROJECT)/cgen/cfgdefault \
    $(TOP)/mediatek/custom/common/cgen/cfgfileinc

LOCAL_SRC_FILES := \
    qcinvram.c

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libfile_op \
    libnvram \
    libdl

LOCAL_MODULE := qcinvram

include $(BUILD_EXECUTABLE)
