LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES:= \
    $(MTK_PATH_SOURCE)/external/nvram/libnvram \
    $(MTK_PATH_SOURCE)/external/nvram/libfile_op \
    $(TOP)/mediatek/custom/$(MTK_PROJECT)/cgen/inc \
    $(TOP)/mediatek/custom/$(MTK_PROJECT)/cgen/cfgdefault

LOCAL_SRC_FILES := \
    qcinvram_lite.c

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libfile_op \
    libnvram

LOCAL_MODULE := qcinvram_lite

#LOCAL_PRELINK_MODULE := false

#LOCAL_ARM_MODE := arm

include $(BUILD_EXECUTABLE)
