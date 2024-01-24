/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_EXYNOS_HWC_MODULE_H_
#define ANDROID_EXYNOS_HWC_MODULE_H_
#include <hardware/hwcomposer.h>
#include <decon_8890.h>

#define HWC_VERSION     HWC_DEVICE_API_VERSION_1_5
#define IDMA_SECURE IDMA_G0

#define VSYNC_DEV_PREFIX "/sys/devices/"
#define VSYNC_DEV_MIDDLE ""
#define VSYNC_DEV_NAME  "13960000.decon_f/vsync"

#define FIMD_WORD_SIZE_BYTES   16
#define FIMD_BURSTLEN   16

#define WINUPDATE_MIN_HEIGHT 16

#define TRY_SECOND_VSYNC_DEV
#ifdef TRY_SECOND_VSYNC_DEV
#define VSYNC_DEV_NAME2  "exynos5-fb.1/vsync"
#define VSYNC_DEV_MIDDLE2  "platform/exynos-sysmmu.30/exynos-sysmmu.11/"
#endif

#define HDMI_RESERVE_MEM_DEV_NAME "/sys/class/ion_cma/ion_video_ext/isolated"
#define SMEM_PATH "/dev/s5p-smem"
#define SECMEM_IOC_SET_VIDEO_EXT_PROC   _IOWR('S', 13, int)

/* Decon WB */
#define DECON_WB_DEV_NAME "/dev/graphics/fb2"
#define DECON_PAD_WB 8
#define DECON_WB_SUBDEV_NAME "exynos-decon2"
#define DECON_EXT_BASE_WINDOW 0
#define DECON_2_MAX_OVERLAY_COUNT    4

/* Max number windows available in Exynos8890 is 8. */
#define NUM_AVAILABLE_HW_WINDOWS	8

const size_t GSC_DST_CROP_W_ALIGNMENT_RGB888 = 1;
const size_t GSC_W_ALIGNMENT = 16;
const size_t GSC_H_ALIGNMENT = 16;
const size_t GSC_DST_H_ALIGNMENT_RGB888 = 1;

const int AVAILABLE_GSC_UNITS[] = { 0, 0, 0, 0, 0, 0};

#define MPP_VG          0
#define MPP_VGR         2
#define MPP_MSC         4
#define MPP_VPP_G       10

const size_t FIMD_EXT_MPP_IDX = 0;
const size_t HDMI_EXT_MPP_IDX = 1;
const size_t WFD_EXT_MPP_IDX = 2;

struct exynos_mpp_t {
    int type;
    unsigned int index;
};

/* VPP_G0 is secure dma, It is managed as IDMA_SECURE separately*/
const exynos_mpp_t AVAILABLE_INTERNAL_MPP_UNITS[] = {{MPP_VPP_G, 1}, {MPP_VPP_G, 2}, {MPP_VPP_G, 3}, {MPP_VG, 0}, {MPP_VG, 1}, {MPP_VGR, 0}, {MPP_VGR, 1}};
const exynos_mpp_t AVAILABLE_EXTERNAL_MPP_UNITS[] = {{MPP_MSC, 0}, {MPP_MSC, 0}, {MPP_MSC, 0}, {MPP_MSC, 0}, {MPP_MSC, 0}};

#define DEFAULT_MPP_DST_FORMAT HAL_PIXEL_FORMAT_RGBX_8888

#endif
