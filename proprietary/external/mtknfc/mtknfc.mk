ifeq ($(MTK_NFC_SUPPORT), yes)

LOCAL_PATH:= mediatek/external/mtknfc

########################################
# MTK NFC Clock Type & Rate Configuration
########################################

ifeq ($(wildcard $(MTK_ROOT_CONFIG_OUT)/nfc.cfg),)
        PRODUCT_COPY_FILES += $(LOCAL_PATH)/nfc.cfg:system/etc/nfc.cfg
else
        PRODUCT_COPY_FILES += $(MTK_ROOT_CONFIG_OUT)/nfc.cfg:system/etc/nfc.cfg
endif

ifeq ($(wildcard $(MTK_ROOT_CONFIG_OUT)/MTKNfclicense.lic),)
        PRODUCT_COPY_FILES += $(LOCAL_PATH)/MTKNfclicense.lic:data/misc/MTKNfclicense.lic
else
        PRODUCT_COPY_FILES += $(MTK_ROOT_CONFIG_OUT)/MTKNfclicense.lic:data/misc/MTKNfclicense.lic
endif

endif

