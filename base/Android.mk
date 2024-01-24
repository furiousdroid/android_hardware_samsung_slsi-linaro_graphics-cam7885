# Copyright (C) 2015 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ifneq ($(findstring exynos, $(TARGET_SOC_NAME)), $(findstring s5e, $(TARGET_SOC_NAME)))
build_dirs :=  \
    libfimg    \
    libacryl \
    libmemtrack \
    giantmscl \
    libdrmresource

ifeq ($(TARGET_USES_UNIVERSAL_LIBHWJPEG), true)
build_dirs +=  \
    libhwjpeg
endif

ifdef BOARD_HWC_VERSION
build_dirs += $(BOARD_HWC_VERSION)

ifeq ($(BOARD_HWC_VERSION), hwc3)
# HWC3 depends on libexynosdisplay in libhwc2.1
build_dirs += libhwc2.1
endif

ifeq ($(BOARD_HWC_VERSION), libhwc1)
# HWC1 has a hard dependency on libexynosgscaler
BOARD_USES_LEGACY_LIBSCALER := true

build_dirs += libmpp
endif
endif

ifeq ($(BOARD_USES_LEGACY_LIBSCALER), true)
build_dirs +=  \
    legacy_libscaler \
    legacy_libgscaler
else
build_dirs +=  \
    libscaler
endif

include $(call all-named-subdir-makefiles,$(build_dirs))
endif
