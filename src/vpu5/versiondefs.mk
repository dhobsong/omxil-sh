#This file defines any version specific defines to enable/disable or
#change modes of options that vary by VPU version.
#
#define VPU "series's". i.e. groups of VPU versions that share the majority
#of settings

ifeq ($(PRODUCT_VPU_VERSION), VPU_VERSION_5)
	VPU_SERIES := VPU5_SERIES
endif
ifeq ($(PRODUCT_VPU_VERSION), VPU_VERSION_5HA)
	VPU_SERIES := VPU5HA_SERIES
endif
ifeq ($(PRODUCT_VPU_VERSION), VPU_VERSION_5HD)
	VPU_SERIES := VPU5HA_SERIES
endif

ifeq ($(VPU_SERIES),)
$(error Unknown VPU setting: $(PRODUCT_VPU_VERSION))
endif

VPU_VERSION_DEFS += -D$(PRODUCT_VPU_VERSION) -D$(VPU_SERIES)
