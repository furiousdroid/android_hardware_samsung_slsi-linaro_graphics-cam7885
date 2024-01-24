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
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utils/Trace.h>
#include <hardware_legacy/uevent.h>

#ifdef HWC_SERVICES
#include "ExynosHWCService.h"
namespace android {
class ExynosHWCService;
}
#endif

#ifdef IP_SERVICE
#include "ExynosIPService.h"
#endif

#include "ExynosHWC.h"
#include "ExynosHWCUtils.h"
#include "ExynosMPPModule.h"
#include "ExynosOverlayDisplay.h"
#include "ExynosExternalDisplayModule.h"
#include "ExynosPrimaryDisplay.h"
#ifdef USES_VIRTUAL_DISPLAY
#include "ExynosVirtualDisplayModule.h"
#endif

void doPSRExit(struct exynos5_hwc_composer_device_1_t *pdev)
{
    int val;
    int ret;
    if (pdev->psrMode != PSR_NONE && pdev->notifyPSRExit) {
        pdev->notifyPSRExit = false;
        ret = ioctl(pdev->primaryDisplay->mDisplayFd, S3CFB_WIN_PSR_EXIT, &val);
    }
}

void exynos5_boot_finished(exynos5_hwc_composer_device_1_t *dev)
{
    ALOGD("Boot Finished");
    int sw_fd;
    exynos5_hwc_composer_device_1_t *pdev =
            (exynos5_hwc_composer_device_1_t *)dev;
    if (pdev == NULL) {
        ALOGE("%s:: dev is NULL", __func__);
        return;
    }
    sw_fd = open("/sys/class/switch/hdmi/state", O_RDONLY);

    if (sw_fd >= 0) {
        char val;
        if (read(sw_fd, &val, 1) == 1 && val == '1') {
            if (pdev->hdmi_hpd != 1) {
                pdev->hdmi_hpd = true;
                if ((pdev->externalDisplay->openHdmi() > 0) && pdev->externalDisplay->getConfig()) {
                    ALOGE("Error reading HDMI configuration");
                    pdev->hdmi_hpd = false;
                }
                pdev->externalDisplay->mBlanked = false;
                if (pdev->procs) {
                    pdev->procs->hotplug(pdev->procs, HWC_DISPLAY_EXTERNAL, true);
                    pdev->procs->invalidate(pdev->procs);
                }
            }
        }
        close(sw_fd);
    }
}

int exynos5_prepare(hwc_composer_device_1_t *dev,
        size_t numDisplays, hwc_display_contents_1_t** displays)
{
    ATRACE_CALL();
    if (!numDisplays || !displays)
        return 0;

    exynos5_hwc_composer_device_1_t *pdev =
            (exynos5_hwc_composer_device_1_t *)dev;
    hwc_display_contents_1_t *fimd_contents = displays[HWC_DISPLAY_PRIMARY];

    hwc_display_contents_1_t *hdmi_contents = displays[HWC_DISPLAY_EXTERNAL];
#ifdef USES_VIRTUAL_DISPLAY
    hwc_display_contents_1_t *virtual_contents = displays[HWC_DISPLAY_VIRTUAL];
    if (virtual_contents == NULL)
        pdev->virtualDisplay->deInit();
#ifdef USES_VIRTUAL_DISPLAY_DECON_EXT_WB
    if (virtual_contents)
        pdev->virtualDisplay->init(virtual_contents);
#endif
#endif
    pdev->updateCallCnt++;
    pdev->update_event_cnt++;
    pdev->LastUpdateTimeStamp = systemTime(SYSTEM_TIME_MONOTONIC);
    pdev->primaryDisplay->getCompModeSwitch();
    pdev->totPixels = 0;
    pdev->incomingPixels = 0;

    pdev->externalDisplay->setHdmiStatus(pdev->hdmi_hpd);

    if (pdev->hwc_ctrl.dynamic_recomp_mode == true &&
        pdev->update_stat_thread_flag == false &&
        pdev->primaryDisplay->mBlanked == false) {
        exynos5_create_update_stat_thread(pdev);
    }

#ifdef USES_VPP
    pdev->mDisplayResourceManager->assignResources(numDisplays, displays);
#endif

    if (fimd_contents) {
        android::Mutex::Autolock lock(pdev->primaryDisplay->mLayerInfoMutex);
        int err = pdev->primaryDisplay->prepare(fimd_contents);
        if (err)
            return err;
    }
    if (hdmi_contents) {
        android::Mutex::Autolock lock(pdev->externalDisplay->mLayerInfoMutex);
        int err = 0;
            err = pdev->externalDisplay->prepare(hdmi_contents);
        if (err)
            return err;
    }

#ifdef USES_VIRTUAL_DISPLAY
    if (virtual_contents) {
#ifdef USES_VIRTUAL_DISPLAY_DECON_EXT_WB
        ExynosVirtualDisplayModule *virDisplay = (ExynosVirtualDisplayModule *)pdev->virtualDisplay;
        virDisplay->setPriContents(fimd_contents);
#endif
        int err = pdev->virtualDisplay->prepare(virtual_contents);
        if (err)
            return err;
    }
#endif

    return 0;
}

int exynos5_set(struct hwc_composer_device_1 *dev,
        size_t numDisplays, hwc_display_contents_1_t** displays)
{
    ATRACE_CALL();
    if (!numDisplays || !displays)
        return 0;

    exynos5_hwc_composer_device_1_t *pdev =
            (exynos5_hwc_composer_device_1_t *)dev;
    hwc_display_contents_1_t *fimd_contents = displays[HWC_DISPLAY_PRIMARY];
    hwc_display_contents_1_t *hdmi_contents = displays[HWC_DISPLAY_EXTERNAL];
#ifdef USES_VIRTUAL_DISPLAY
    hwc_display_contents_1_t *virtual_contents = displays[HWC_DISPLAY_VIRTUAL];
    int virtual_err = 0;
#endif
    int fimd_err = 0, hdmi_err = 0;

    if (fimd_contents) {
        android::Mutex::Autolock lock(pdev->primaryDisplay->mLayerInfoMutex);
        fimd_err = pdev->primaryDisplay->set(fimd_contents);
    }

    if (pdev->mS3DMode != S3D_MODE_STOPPING && !pdev->mHdmiResolutionHandled) {
        pdev->mHdmiResolutionHandled = true;
        pdev->hdmi_hpd = true;
        pdev->externalDisplay->enable();
        if (pdev->procs) {
            pdev->procs->hotplug(pdev->procs, HWC_DISPLAY_EXTERNAL, true);
            pdev->procs->invalidate(pdev->procs);
        }
    }

    if (hdmi_contents && fimd_contents) {
        android::Mutex::Autolock lock(pdev->externalDisplay->mLayerInfoMutex);
        hdmi_err = pdev->externalDisplay->set(hdmi_contents);
    }

    if (pdev->hdmi_hpd && pdev->mHdmiResolutionChanged) {
        if (pdev->mS3DMode == S3D_MODE_DISABLED && pdev->externalDisplay->isPresetSupported(pdev->mHdmiPreset))
            pdev->externalDisplay->setPreset(pdev->mHdmiPreset);
    }
    if (pdev->mS3DMode == S3D_MODE_STOPPING) {
        pdev->mS3DMode = S3D_MODE_DISABLED;
#ifndef USES_VPP
        for (int i = 0; i < pdev->primaryDisplay->mNumMPPs; i++)
            pdev->primaryDisplay->mMPPs[i]->mS3DMode = S3D_NONE;

        if (pdev->externalDisplay->mMPPs[0] != NULL)
            pdev->externalDisplay->mMPPs[0]->mS3DMode = S3D_NONE;
#endif
    }

#ifdef USES_VIRTUAL_DISPLAY
    if (virtual_contents && fimd_contents)
        virtual_err = pdev->virtualDisplay->set(virtual_contents);
#endif

#ifdef EXYNOS_SUPPORT_PSR_EXIT
    pdev->notifyPSRExit = true;
#else
    pdev->notifyPSRExit = false;
#endif

    pdev->primaryDisplay->freeMPP();

#ifdef USES_VPP
    pdev->mDisplayResourceManager->cleanupMPPs();
#endif

    if (fimd_err)
        return fimd_err;

#ifndef USES_VIRTUAL_DISPLAY
    return hdmi_err;
#else
    if (hdmi_err)
        return hdmi_err;

    return virtual_err;
#endif
}

void exynos5_registerProcs(struct hwc_composer_device_1* dev,
        hwc_procs_t const* procs)
{
    struct exynos5_hwc_composer_device_1_t* pdev =
            (struct exynos5_hwc_composer_device_1_t*)dev;
    pdev->procs = procs;
}

int exynos5_query(struct hwc_composer_device_1* dev, int what, int *value)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
            (struct exynos5_hwc_composer_device_1_t *)dev;

    switch (what) {
    case HWC_BACKGROUND_LAYER_SUPPORTED:
        // we do not support the background layer
        value[0] = 0;
        break;
    case HWC_VSYNC_PERIOD:
        // vsync period in nanosecond
        value[0] = pdev->primaryDisplay->mVsyncPeriod;
        break;
#ifdef USES_VIRTUAL_DISPLAY
    case HWC_DISPLAY_TYPES_SUPPORTED:
        // support virtual display
        value[0] |= HWC_DISPLAY_VIRTUAL_BIT;
        break;
#endif
    default:
        // unsupported query
        return -EINVAL;
    }
    return 0;
}

int exynos5_eventControl(struct hwc_composer_device_1 *dev, int __unused dpy,
        int event, int enabled)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
            (struct exynos5_hwc_composer_device_1_t *)dev;

    switch (event) {
    case HWC_EVENT_VSYNC:
        __u32 val = !!enabled;
        pdev->VsyncInterruptStatus = val;
        int err = ioctl(pdev->primaryDisplay->mDisplayFd, S3CFB_SET_VSYNC_INT, &val);
        if (err < 0) {
            ALOGE("vsync ioctl failed");
            return -errno;
        }
        return 0;
    }

    return -EINVAL;
}

void handle_hdmi_uevent(struct exynos5_hwc_composer_device_1_t *pdev,
        const char *buff, int len)
{
    const char *s = buff;
    s += strlen(s) + 1;

    while (*s) {
        if (!strncmp(s, "SWITCH_STATE=", strlen("SWITCH_STATE=")))
            pdev->hdmi_hpd = atoi(s + strlen("SWITCH_STATE=")) == 1;

        s += strlen(s) + 1;
        if (s - buff >= len)
            break;
    }

    if (pdev->hdmi_hpd) {
        if ((pdev->externalDisplay->openHdmi() > 0) && pdev->externalDisplay->getConfig()) {
            ALOGE("Error reading HDMI configuration");
            pdev->hdmi_hpd = false;
            return;
        }

        pdev->externalDisplay->mBlanked = false;
    }

    ALOGV("HDMI HPD changed to %s", pdev->hdmi_hpd ? "enabled" : "disabled");
    if (pdev->hdmi_hpd)
        ALOGI("HDMI Resolution changed to %dx%d",
                pdev->externalDisplay->mXres, pdev->externalDisplay->mYres);

    /* hwc_dev->procs is set right after the device is opened, but there is
     * still a race condition where a hotplug event might occur after the open
     * but before the procs are registered. */
    if (pdev->procs)
        pdev->procs->hotplug(pdev->procs, HWC_DISPLAY_EXTERNAL, pdev->hdmi_hpd);
}

void handle_tui_uevent(struct exynos5_hwc_composer_device_1_t *pdev,
        const char *buff, int len)
{
#ifdef USES_VPP
#ifdef DISABLE_IDMA_SECURE
    return;
#else
    const char *s = buff;
    unsigned int tui_disabled = 1;
    bool useSecureDMA = true;
    s += strlen(s) + 1;

    while (*s) {
        if (!strncmp(s, "SWITCH_STATE=", strlen("SWITCH_STATE=")))
            tui_disabled = atoi(s + strlen("SWITCH_STATE=")) == 0;

        s += strlen(s) + 1;
        if (s - buff >= len)
            break;
    }

    if (tui_disabled)
        useSecureDMA = true;
    else
        useSecureDMA = false;

    ALOGI("TUI mode is %s", tui_disabled ? "disabled" : "enabled");

    if (pdev->primaryDisplay->mUseSecureDMA != useSecureDMA) {
        pdev->primaryDisplay->mUseSecureDMA = useSecureDMA;
        if ((pdev->procs) && (pdev->procs->invalidate))
            pdev->procs->invalidate(pdev->procs);
    }
#endif
#endif
}

void handle_vsync_event(struct exynos5_hwc_composer_device_1_t *pdev)
{
    if (!pdev->procs)
        return;

    int err = lseek(pdev->vsync_fd, 0, SEEK_SET);
    if (err < 0) {
        ALOGE("error seeking to vsync timestamp: %s", strerror(errno));
        return;
    }

    char buf[4096];
    err = read(pdev->vsync_fd, buf, sizeof(buf));
    if (err < 0) {
        ALOGE("error reading vsync timestamp: %s", strerror(errno));
        return;
    }
    buf[sizeof(buf) - 1] = '\0';

    errno = 0;
    uint64_t timestamp = strtoull(buf, NULL, 0);
    if (!errno)
        pdev->procs->vsync(pdev->procs, 0, timestamp);
}

void *hwc_update_stat_thread(void *data)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
            (struct exynos5_hwc_composer_device_1_t *)data;
    int event_cnt = 0;
    android_atomic_inc(&(pdev->updateThreadStatus));

    while (pdev->update_stat_thread_flag) {
        event_cnt = pdev->update_event_cnt;
        /*
         * If there is no update for more than 100ms, favor the 3D composition mode.
         * If all other conditions are met, mode will be switched to 3D composition.
         */
        usleep(100000);
        if (event_cnt == pdev->update_event_cnt) {
            if (pdev->primaryDisplay->getCompModeSwitch() == HWC_2_GLES) {
                if ((pdev->procs) && (pdev->procs->invalidate)) {
                    pdev->update_event_cnt = 0;
                    pdev->procs->invalidate(pdev->procs);
                }
            }
        }
    }
    android_atomic_dec(&(pdev->updateThreadStatus));
    return NULL;
}

void exynos5_create_update_stat_thread(struct exynos5_hwc_composer_device_1_t *dev)
{
    /* pthread_create shouldn't have ben failed. But, ignore even if some error */
    if (pthread_create(&dev->update_stat_thread, NULL, hwc_update_stat_thread, dev) != 0) {
        ALOGE("%s: failed to start update_stat thread:", __func__);
        dev->update_stat_thread_flag = false;
    } else {
        dev->update_stat_thread_flag = true;
    }
}

void *hwc_vsync_thread(void *data)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
            (struct exynos5_hwc_composer_device_1_t *)data;
    char uevent_desc[4096];
    memset(uevent_desc, 0, sizeof(uevent_desc));

    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);

    uevent_init();

    char temp[4096];
    int err = read(pdev->vsync_fd, temp, sizeof(temp));
    if (err < 0) {
        ALOGE("error reading vsync timestamp: %s", strerror(errno));
        return NULL;
    }

    struct pollfd fds[2];
    fds[0].fd = pdev->vsync_fd;
    fds[0].events = POLLPRI;
    fds[1].fd = uevent_get_fd();
    fds[1].events = POLLIN;

    while (true) {
        int err = poll(fds, 2, -1);

        if (err > 0) {
            if (fds[0].revents & POLLPRI) {
                handle_vsync_event(pdev);
            }
            else if (fds[1].revents & POLLIN) {
                int len = uevent_next_event(uevent_desc,
                        sizeof(uevent_desc) - 2);

                bool hdmi = !strcmp(uevent_desc,
                        "change@/devices/virtual/switch/hdmi");
                bool tui_status = !strcmp(uevent_desc,
                        "change@/devices/virtual/switch/tui");

                if (hdmi)
                    handle_hdmi_uevent(pdev, uevent_desc, len);
                else if (tui_status)
                    handle_tui_uevent(pdev, uevent_desc, len);
            }
        }
        else if (err == -1) {
            if (errno == EINTR)
                break;
            ALOGE("error in vsync thread: %s", strerror(errno));
        }
    }

    return NULL;
}

int exynos5_blank(struct hwc_composer_device_1 *dev, int disp, int blank)
{
    ATRACE_CALL();
    struct exynos5_hwc_composer_device_1_t *pdev =
            (struct exynos5_hwc_composer_device_1_t *)dev;
#ifdef SKIP_DISPLAY_BLANK_CTRL
    return 0;
#endif
    ALOGI("%s:: disp(%d), blank(%d)", __func__, disp, blank);
    switch (disp) {
    case HWC_DISPLAY_PRIMARY: {
        int fb_blank = blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK;
        if (fb_blank == FB_BLANK_POWERDOWN) {
            int fence = pdev->primaryDisplay->clearDisplay();
            if (fence < 0) {
                HLOGE("error clearing primary display");
            } else {
#ifndef USES_VPP
                if (pdev->primaryDisplay->mGscUsed && pdev->primaryDisplay->mMPPs[FIMD_GSC_IDX]->isOTF())
                    pdev->primaryDisplay->mMPPs[FIMD_GSC_IDX]->cleanupOTF();
#endif
                close(fence);
            }
        }
#if !defined(HDMI_ON_IN_SUSPEND) && defined(CHANGE_POWEROFF_SEQ)
	/*
	 * LCD power block shouldn't be turned off
	 * before TV power block is turned off in Exynos4.
	 */
        if (pdev->hdmi_hpd) {
            if (blank && !pdev->externalDisplay->mBlanked) {
                pdev->externalDisplay->disable();
            }
            pdev->externalDisplay->mBlanked = !!blank;
        }
#endif
        pdev->primaryDisplay->mBlanked = !!blank;

        android_atomic_acquire_load(&pdev->updateThreadStatus);

        if(pdev->updateThreadStatus != 0) {
            if (fb_blank == FB_BLANK_POWERDOWN) {
                pdev->update_stat_thread_flag = false;
                pthread_join(pdev->update_stat_thread, 0);
            }
        } else { // thread is not alive
            if (fb_blank == FB_BLANK_UNBLANK && pdev->hwc_ctrl.dynamic_recomp_mode == true)
                exynos5_create_update_stat_thread(pdev);
        }
        int err = ioctl(pdev->primaryDisplay->mDisplayFd, FBIOBLANK, fb_blank);
        if (err < 0) {
            if (errno == EBUSY)
                ALOGI("%sblank ioctl failed (display already %sblanked)",
                        blank ? "" : "un", blank ? "" : "un");
            else
                ALOGE("%sblank ioctl failed: %s", blank ? "" : "un",
                        strerror(errno));
            return -errno;
        }
        break;
    }
    case HWC_DISPLAY_EXTERNAL:
#if !defined(HDMI_ON_IN_SUSPEND) && !defined(CHANGE_POWEROFF_SEQ)
        if (pdev->hdmi_hpd) {
            if (blank && !pdev->externalDisplay->mBlanked) {
                pdev->externalDisplay->disable();
            }
            pdev->externalDisplay->mBlanked = !!blank;
        }
#else
        fence = pdev->externalDisplay->clearDisplay();
        if (fence >= 0)
            close(fence);
        pdev->externalDisplay->mBlanked = !!blank;
#endif
        break;

    default:
        return -EINVAL;

    }

    return 0;
}

void exynos5_dump(hwc_composer_device_1* dev, char *buff, int buff_len)
{
    if (buff_len <= 0)
        return;

    struct exynos5_hwc_composer_device_1_t *pdev =
            (struct exynos5_hwc_composer_device_1_t *)dev;

    android::String8 result;

    result.appendFormat("\n  hdmi_enabled=%u\n", pdev->externalDisplay->mEnabled);
    if (pdev->externalDisplay->mEnabled)
        result.appendFormat("    w=%u, h=%u\n", pdev->externalDisplay->mXres, pdev->externalDisplay->mYres);

    result.append("Primary device's config information\n");
    pdev->primaryDisplay->dump(result);

#ifdef USES_VPP
    if (pdev->hdmi_hpd) {
        result.append("\n");
        result.append("External device's config information\n");
        pdev->externalDisplay->dump(result);
    }
#ifdef USES_VIRTUAL_DISPLAY_DECON_EXT_WB
    if (pdev->virtualDisplay->mIsWFDState) {
        result.append("\n");
        result.append("Virtual device's config information\n");
        pdev->virtualDisplay->dump(result);
    }
#endif
#endif
    {
        android::Mutex::Autolock lock(pdev->primaryDisplay->mLayerInfoMutex);
        result.append("\n");
        result.append("Primary device's layer information\n");
        pdev->primaryDisplay->dumpLayerInfo(result);
    }

    if (pdev->hdmi_hpd) {
        android::Mutex::Autolock lock(pdev->externalDisplay->mLayerInfoMutex);
        result.append("\n");
        result.append("External device's layer information\n");
        pdev->externalDisplay->dumpLayerInfo(result);
    }
#if USES_VIRTUAL_DISPLAY_DECON_EXT_WB
    if (pdev->virtualDisplay->mIsWFDState) {
        android::Mutex::Autolock lock(pdev->virtualDisplay->mLayerInfoMutex);
        result.append("\n");
        result.append("Virtual device's layer information\n");
        pdev->virtualDisplay->dumpLayerInfo(result);
    }
#endif
    strlcpy(buff, result.string(), buff_len);
}

int exynos5_getDisplayConfigs(struct hwc_composer_device_1 *dev,
        int disp, uint32_t *configs, size_t *numConfigs)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
               (struct exynos5_hwc_composer_device_1_t *)dev;
    int err = 0;

    if (*numConfigs == 0)
        return 0;

    if (disp == HWC_DISPLAY_PRIMARY) {
        configs[0] = 0;
        *numConfigs = 1;
        return 0;
    } else if (disp == HWC_DISPLAY_EXTERNAL) {
        if (!pdev->hdmi_hpd) {
            return -EINVAL;
        }
        if (hwcHasApiVersion((hwc_composer_device_1_t*)dev, HWC_DEVICE_API_VERSION_1_4))
            err = pdev->externalDisplay->getDisplayConfigs(configs, numConfigs);
        else {
            err = pdev->externalDisplay->getConfig();
            configs[0] = 0;
            *numConfigs = 1;
        }
        if (err) {
            return -EINVAL;
        }
        return 0;
#ifdef USES_VIRTUAL_DISPLAY
    } else if (disp == HWC_DISPLAY_VIRTUAL) {
        int err = pdev->virtualDisplay->getConfig();
        if (err) {
            return -EINVAL;
        }
        configs[0] = 0;
        *numConfigs = 1;
        return 0;
#endif
    }

    return -EINVAL;
}

int32_t exynos5_hdmi_attribute(struct exynos5_hwc_composer_device_1_t *pdev,
        const uint32_t attribute)
{
    switch(attribute) {
    case HWC_DISPLAY_VSYNC_PERIOD:
        return pdev->primaryDisplay->mVsyncPeriod;

    case HWC_DISPLAY_WIDTH:
        return pdev->externalDisplay->mXres;

    case HWC_DISPLAY_HEIGHT:
        return pdev->externalDisplay->mYres;

    case HWC_DISPLAY_DPI_X:
    case HWC_DISPLAY_DPI_Y:
        return 0; // unknown

    default:
        ALOGE("unknown display attribute %u", attribute);
        return -EINVAL;
    }
}

int exynos5_getDisplayAttributes(struct hwc_composer_device_1 *dev,
        int disp, uint32_t config, const uint32_t *attributes, int32_t *values)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
                   (struct exynos5_hwc_composer_device_1_t *)dev;

    for (int i = 0; attributes[i] != HWC_DISPLAY_NO_ATTRIBUTE; i++) {
        if (disp == HWC_DISPLAY_PRIMARY)
            values[i] = pdev->primaryDisplay->getDisplayAttributes(attributes[i]);
        else if (disp == HWC_DISPLAY_EXTERNAL) {
            if (hwcHasApiVersion((hwc_composer_device_1_t*)dev, HWC_DEVICE_API_VERSION_1_4))
                values[i] = pdev->externalDisplay->getDisplayAttributes(attributes[i], config);
            else
                values[i] = exynos5_hdmi_attribute(pdev, attributes[i]);
        }
#ifdef USES_VIRTUAL_DISPLAY
        else if (disp == HWC_DISPLAY_VIRTUAL)
            values[i] = pdev->virtualDisplay->getDisplayAttributes(attributes[i]);
#endif
        else {
            ALOGE("unknown display type %u", disp);
            return -EINVAL;
        }
    }

    return 0;
}

int exynos_getActiveConfig(struct hwc_composer_device_1* dev, int disp)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
                   (struct exynos5_hwc_composer_device_1_t *)dev;
    if (disp == HWC_DISPLAY_PRIMARY)
        return 0;
    else if (disp == HWC_DISPLAY_EXTERNAL) {
        if (pdev->hdmi_hpd) {
            if (hwcHasApiVersion((hwc_composer_device_1_t*)dev, HWC_DEVICE_API_VERSION_1_4))
                return  pdev->externalDisplay->getActiveConfig();
            else
                return 0;
        } else {
            ALOGE("%s::External device is not connected", __func__);
            return -1;
        }
    } else if (disp == HWC_DISPLAY_VIRTUAL)
        return 0;
    else {
        ALOGE("%s:: unknown display type %u", __func__, disp);
        return -EINVAL;
    }
}

int exynos_setActiveConfig(struct hwc_composer_device_1* dev, int disp, int index)
{
    struct exynos5_hwc_composer_device_1_t *pdev =
                   (struct exynos5_hwc_composer_device_1_t *)dev;
    ALOGI("%s:: disp(%d), index(%d)", __func__, disp, index);
    if (disp == HWC_DISPLAY_PRIMARY) {
        if (index != 0) {
            ALOGE("%s::Primary display doen't support index(%d)", __func__, index);
            return -1;
        }
        return 0;
    }
    else if (disp == HWC_DISPLAY_EXTERNAL) {
        if (pdev->hdmi_hpd) {
            if (hwcHasApiVersion((hwc_composer_device_1_t*)dev, HWC_DEVICE_API_VERSION_1_4)) {
                return pdev->externalDisplay->setActiveConfig(index);
            } else {
                if (index != 0) {
                    ALOGE("%s::External display doen't support index(%d)", __func__, index);
                    return -1;
                } else {
                    return 0;
                }
            }
        } else {
            ALOGE("%s::External device is not connected", __func__);
            return -1;
        }
    } else if (disp == HWC_DISPLAY_VIRTUAL)
        return 0;

    return -1;
}

int exynos_setCursorPositionAsync(struct hwc_composer_device_1 __unused *dev, int __unused disp, int __unused x_pos, int __unused y_pos)
{
    return 0;
}

int exynos_setPowerMode(struct hwc_composer_device_1* dev, int disp, int mode)
{
    ATRACE_CALL();
    struct exynos5_hwc_composer_device_1_t *pdev =
            (struct exynos5_hwc_composer_device_1_t *)dev;
#ifdef SKIP_DISPLAY_BLANK_CTRL
    return 0;
#endif
    ALOGI("%s:: disp(%d), mode(%d)", __func__, disp, mode);
    int fb_blank = 0;
    int blank = 0;
    if (mode == HWC_POWER_MODE_OFF) {
        fb_blank = FB_BLANK_POWERDOWN;
        blank = 1;
    } else {
        fb_blank = FB_BLANK_UNBLANK;
        blank = 0;
    }

    switch (disp) {
    case HWC_DISPLAY_PRIMARY: {
#ifdef USES_VPP
        if ((mode == HWC_POWER_MODE_DOZE) || (mode == HWC_POWER_MODE_DOZE_SUSPEND)) {
            if (pdev->primaryDisplay->mBlanked == 0) {
                fb_blank = FB_BLANK_POWERDOWN;
                int err = ioctl(pdev->primaryDisplay->mDisplayFd, FBIOBLANK, fb_blank);
                if (err < 0) {
                    ALOGE("blank ioctl failed: %s, mode(%d)", strerror(errno), mode);
                    return -errno;
                }
            }
            pdev->primaryDisplay->mBlanked = 1;
            return pdev->primaryDisplay->setPowerMode(mode);
        }
#endif
        if (fb_blank == FB_BLANK_POWERDOWN) {
            int fence = -1;
            fence = pdev->primaryDisplay->clearDisplay();
            if (fence < 0) {
                HLOGE("error clearing primary display");
            } else {
#ifndef USES_VPP
                if (pdev->primaryDisplay->mGscUsed && pdev->primaryDisplay->mMPPs[FIMD_GSC_IDX]->isOTF())
                    pdev->primaryDisplay->mMPPs[FIMD_GSC_IDX]->cleanupOTF();
#endif
                close(fence);
            }
        }
#if !defined(HDMI_ON_IN_SUSPEND) && defined(CHANGE_POWEROFF_SEQ)
	/*
	 * LCD power block shouldn't be turned off
	 * before TV power block is turned off in Exynos4.
	 */
        if (pdev->hdmi_hpd) {
            if (blank && !pdev->externalDisplay->mBlanked) {
                pdev->externalDisplay->disable();
            }
            pdev->externalDisplay->mBlanked = !!blank;
        }
#endif
        pdev->primaryDisplay->mBlanked = !!blank;
        if (android_atomic_acquire_load(&pdev->updateThreadStatus) != 0) {
            if (fb_blank == FB_BLANK_POWERDOWN) {
                pdev->update_stat_thread_flag = false;
                pthread_join(pdev->update_stat_thread, 0);
            }
        } else { // thread is not alive
            if (fb_blank == FB_BLANK_UNBLANK && pdev->hwc_ctrl.dynamic_recomp_mode == true)
                exynos5_create_update_stat_thread(pdev);
        }
        int err = ioctl(pdev->primaryDisplay->mDisplayFd, FBIOBLANK, fb_blank);
        if (err < 0) {
            if (errno == EBUSY)
                ALOGI("%sblank ioctl failed (display already %sblanked)",
                        blank ? "" : "un", blank ? "" : "un");
            else
                ALOGE("%sblank ioctl failed: %s", blank ? "" : "un",
                        strerror(errno));
            return -errno;
        }
        break;
    }
    case HWC_DISPLAY_EXTERNAL:
#if !defined(HDMI_ON_IN_SUSPEND) && !defined(CHANGE_POWEROFF_SEQ)
        if (pdev->hdmi_hpd) {
            if (blank && !pdev->externalDisplay->mBlanked) {
                pdev->externalDisplay->disable();
            }
            pdev->externalDisplay->mBlanked = !!blank;
        }
#else
        fence = pdev->externalDisplay->clearDisplay();
        if (fence >= 0)
            close(fence);
        pdev->externalDisplay->mBlanked = !!blank;
#endif
        break;

    default:
        return -EINVAL;

    }

    return 0;
}

int exynos5_close(hw_device_t* device);

int exynos5_open(const struct hw_module_t *module, const char *name,
        struct hw_device_t **device)
{
    int ret = 0;
    int refreshRate;

    if (strcmp(name, HWC_HARDWARE_COMPOSER)) {
        return -EINVAL;
    }

    struct exynos5_hwc_composer_device_1_t *dev;
    dev = (struct exynos5_hwc_composer_device_1_t *)malloc(sizeof(*dev));
    memset(dev, 0, sizeof(*dev));

    dev->primaryDisplay = new ExynosPrimaryDisplay(NUM_GSC_UNITS, dev);
    dev->externalDisplay = new ExynosExternalDisplayModule(dev);
#ifdef USES_VIRTUAL_DISPLAY
    dev->virtualDisplay = new ExynosVirtualDisplayModule(dev);
#endif
    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
            (const struct hw_module_t **)&dev->primaryDisplay->mGrallocModule)) {
        ALOGE("failed to get gralloc hw module");
        ret = -EINVAL;
        goto err_get_module;
    }

    if (gralloc_open((const hw_module_t *)dev->primaryDisplay->mGrallocModule,
            &dev->primaryDisplay->mAllocDevice)) {
        ALOGE("failed to open gralloc");
        ret = -EINVAL;
        goto err_get_module;
    }
    dev->externalDisplay->mAllocDevice = dev->primaryDisplay->mAllocDevice;
#ifdef USES_VIRTUAL_DISPLAY
    dev->virtualDisplay->mAllocDevice = dev->primaryDisplay->mAllocDevice;
#endif

#ifdef HDMI_PRIMARY_DISPLAY
    dev->primaryDisplay->mDisplayFd = open("/dev/graphics/fb2", O_RDWR);
    if (dev->primaryDisplay->mDisplayFd < 0)
		dev->primaryDisplay->mDisplayFd = open("/dev/graphics/fb1", O_RDWR);
#else
    dev->primaryDisplay->mDisplayFd = open("/dev/graphics/fb0", O_RDWR);
#endif
    if (dev->primaryDisplay->mDisplayFd < 0) {
        ALOGE("failed to open framebuffer");
        ret = dev->primaryDisplay->mDisplayFd;
        goto err_open_fb;
    }

    struct fb_var_screeninfo info;
    if (ioctl(dev->primaryDisplay->mDisplayFd, FBIOGET_VSCREENINFO, &info) == -1) {
        ALOGE("FBIOGET_VSCREENINFO ioctl failed: %s", strerror(errno));
        ret = -errno;
        goto err_ioctl;
    }

    if (info.reserved[0] == 0 && info.reserved[1] == 0) {
        /* save physical lcd width, height to reserved[] */
        info.reserved[0] = info.xres;
        info.reserved[1] = info.yres;

        if (ioctl(dev->primaryDisplay->mDisplayFd, FBIOPUT_VSCREENINFO, &info) == -1) {
            ALOGE("FBIOPUT_VSCREENINFO ioctl failed: %s", strerror(errno));
            ret = -errno;
            goto err_ioctl;
        }
    }

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.hwc.force_gpu", value, "0");
    dev->force_gpu = atoi(value);

    /* restore physical lcd width, height from reserved[] */
    int lcd_xres, lcd_yres;
    lcd_xres = info.reserved[0];
    lcd_yres = info.reserved[1];

    refreshRate = 1000000000000LLU /
        (
         uint64_t( info.upper_margin + info.lower_margin + lcd_yres + info.vsync_len )
         * ( info.left_margin  + info.right_margin + lcd_xres + info.hsync_len )
         * info.pixclock
        );

    if (refreshRate == 0) {
        ALOGW("invalid refresh rate, assuming 60 Hz");
        refreshRate = 60;
    }

    dev->primaryDisplay->mXres = lcd_xres;
    dev->primaryDisplay->mYres = lcd_yres;
    dev->primaryDisplay->mXdpi = 1000 * (lcd_xres * 25.4f) / info.width;
    dev->primaryDisplay->mYdpi = 1000 * (lcd_yres * 25.4f) / info.height;
    dev->primaryDisplay->mVsyncPeriod  = 1000000000 / refreshRate;

    ALOGD("using\n"
          "xres         = %d px\n"
          "yres         = %d px\n"
          "width        = %d mm (%f dpi)\n"
          "height       = %d mm (%f dpi)\n"
          "refresh rate = %d Hz\n",
          dev->primaryDisplay->mXres, dev->primaryDisplay->mYres, info.width, dev->primaryDisplay->mXdpi / 1000.0,
          info.height, dev->primaryDisplay->mYdpi / 1000.0, refreshRate);
#ifndef USES_VPP
#ifdef FIMD_BW_OVERLAP_CHECK
    fimd_bw_overlap_limits_init(dev->primaryDisplay->mXres, dev->primaryDisplay->mYres,
                    dev->primaryDisplay->mDmaChannelMaxBandwidth, dev->primaryDisplay->mDmaChannelMaxOverlapCount);
#else
    for (size_t i = 0; i < MAX_NUM_FIMD_DMA_CH; i++) {
        dev->primaryDisplay->mDmaChannelMaxBandwidth[i] =2560 * 1600;
        dev->primaryDisplay->mDmaChannelMaxOverlapCount[i] = 1;
    }
#endif
#endif

#ifdef FIMD_WINDOW_OVERLAP_CHECK
    /*
     *  Trivial implementation.
     *  Effective only for checking the case that
     *    mMaxWindowOverlapCnt = (NUM_HW_WINDOWS - 1)
     */
    dev->primaryDisplay->mMaxWindowOverlapCnt =
        fimd_window_overlap_limits_init(dev->primaryDisplay->mXres, dev->primaryDisplay->mYres);
#else
    dev->primaryDisplay->mMaxWindowOverlapCnt = NUM_HW_WINDOWS;
#endif

    if (dev->externalDisplay->openHdmi() < 0) {
        ALOGE("openHdmi fail");
    }

#ifdef USES_VPP
    dev->mDisplayResourceManager = new ExynosDisplayResourceManagerModule(dev);
#endif
    char devname[MAX_DEV_NAME + 1];
    devname[MAX_DEV_NAME] = '\0';

    strncpy(devname, VSYNC_DEV_PREFIX, MAX_DEV_NAME);
    strlcat(devname, VSYNC_DEV_NAME, MAX_DEV_NAME);

    dev->vsync_fd = open(devname, O_RDONLY);
    if (dev->vsync_fd < 0) {
        ALOGI("Failed to open vsync attribute at %s", devname);
        devname[strlen(VSYNC_DEV_PREFIX)] = '\0';
        strlcat(devname, VSYNC_DEV_MIDDLE, MAX_DEV_NAME);
        strlcat(devname, VSYNC_DEV_NAME, MAX_DEV_NAME);
        ALOGI("Retrying with %s", devname);
        dev->vsync_fd = open(devname, O_RDONLY);
    }

#ifdef TRY_SECOND_VSYNC_DEV
    if (dev->vsync_fd < 0) {
        strncpy(devname, VSYNC_DEV_PREFIX, MAX_DEV_NAME);
        strlcat(devname, VSYNC_DEV_NAME2, MAX_DEV_NAME);

        dev->vsync_fd = open(devname, O_RDONLY);
        if (dev->vsync_fd < 0) {
            ALOGI("Failed to open vsync attribute at %s", devname);
            devname[strlen(VSYNC_DEV_PREFIX)] = '\0';
            strlcat(devname, VSYNC_DEV_MIDDLE2, MAX_DEV_NAME);
            strlcat(devname, VSYNC_DEV_NAME2, MAX_DEV_NAME);
            ALOGI("Retrying with %s", devname);
            dev->vsync_fd = open(devname, O_RDONLY);
        }
    }

#endif

    if (dev->vsync_fd < 0) {
        ALOGE("failed to open vsync attribute");
        ret = dev->vsync_fd;
        goto err_hdmi_open;
    } else {
	    struct stat st;
	    if (fstat(dev->vsync_fd, &st) < 0) {
		    ALOGE("Failed to stat vsync node at %s", devname);
		    goto err_vsync_stat;
	    }

	    if (!S_ISREG(st.st_mode)) {
		    ALOGE("vsync node at %s should be a regualar file", devname);
		    goto err_vsync_stat;
	    }
    }

    dev->psrInfoFd = NULL;

    char psrDevname[MAX_DEV_NAME + 1];
    memset(psrDevname, 0, MAX_DEV_NAME + 1);
    strncpy(psrDevname, devname, strlen(devname) - 5);
    strlcat(psrDevname, "psr_info", MAX_DEV_NAME);
    ALOGI("PSR info devname = %s\n", psrDevname);

    dev->psrInfoFd = fopen(psrDevname, "r");
    if (dev->psrInfoFd == NULL) {
        ALOGW("HWC needs to know whether LCD driver is using PSR mode or not\n");
    } else {
        char val[4];
        if (fread(&val, 1, 1, dev->psrInfoFd) == 1) {
            dev->psrMode = (0x03 & atoi(val));
            dev->panelType = ((0x03 << 2) & atoi(val)) >> 2;
        }
    }

    ALOGI("PSR mode   = %d (0: video mode, 1: DP PSR mode, 2: MIPI-DSI command mode)\n",
            dev->psrMode);
    ALOGI("Panel type = %d (0: Legacy, 1: DSC)\n",
            dev->panelType);

#ifdef USES_VPP
    dev->primaryDisplay->mPanelType = dev->panelType;
    if (dev->panelType == PANEL_DSC) {
        uint32_t sliceNum = 0;
        uint32_t sliceSize = 0;
        if (fscanf(dev->psrInfoFd, "\n%d\n%d\n", &sliceNum, &sliceSize) < 0) {
            ALOGE("Fail to read slice information");
        } else {
            dev->primaryDisplay->mDSCHSliceNum = sliceNum;
            dev->primaryDisplay->mDSCYSliceSize = sliceSize;
        }
        ALOGI("DSC H_Slice_Num: %d, Y_Slice_Size: %d", dev->primaryDisplay->mDSCHSliceNum, dev->primaryDisplay->mDSCYSliceSize);
    }
#endif

    dev->base.common.tag = HARDWARE_DEVICE_TAG;
    dev->base.common.version = HWC_VERSION;
    dev->base.common.module = const_cast<hw_module_t *>(module);
    dev->base.common.close = exynos5_close;

    dev->base.prepare = exynos5_prepare;
    dev->base.set = exynos5_set;
    dev->base.eventControl = exynos5_eventControl;
    dev->base.query = exynos5_query;
    dev->base.registerProcs = exynos5_registerProcs;
    dev->base.dump = exynos5_dump;
    dev->base.getDisplayConfigs = exynos5_getDisplayConfigs;
    dev->base.getDisplayAttributes = exynos5_getDisplayAttributes;
    if (hwcHasApiVersion((hwc_composer_device_1_t*)dev, HWC_DEVICE_API_VERSION_1_4)) {
        dev->base.getActiveConfig = exynos_getActiveConfig;
        dev->base.setActiveConfig = exynos_setActiveConfig;
        dev->base.setCursorPositionAsync = exynos_setCursorPositionAsync;
        dev->base.setPowerMode = exynos_setPowerMode;
    } else {
        dev->base.blank = exynos5_blank;
    }

    *device = &dev->base.common;

#ifdef IP_SERVICE
    android::ExynosIPService *mIPService;
    mIPService = android::ExynosIPService::getExynosIPService();
    ret = mIPService->createServiceLocked();
    if (ret < 0)
        goto err_vsync;
#endif

#ifdef HWC_SERVICES
    android::ExynosHWCService   *mHWCService;
    mHWCService = android::ExynosHWCService::getExynosHWCService();
    mHWCService->setExynosHWCCtx(dev);
    mHWCService->setPSRExitCallback(doPSRExit);
#if !defined(HDMI_INCAPABLE)
    mHWCService->setBootFinishedCallback(exynos5_boot_finished);
#endif
#endif

    dev->mHdmiResolutionChanged = false;
    dev->mHdmiResolutionHandled = true;
    dev->mS3DMode = S3D_MODE_DISABLED;
    dev->mHdmiPreset = HDMI_PRESET_DEFAULT;
    dev->mHdmiCurrentPreset = HDMI_PRESET_DEFAULT;
    dev->mUseSubtitles = false;
    dev->notifyPSRExit = false;

    ret = pthread_create(&dev->vsync_thread, NULL, hwc_vsync_thread, dev);
    if (ret) {
        ALOGE("failed to start vsync thread: %s", strerror(ret));
        ret = -ret;
        goto err_vsync;
    }

#ifdef G2D_COMPOSITION
    dev->primaryDisplay->num_of_allocated_lay = 0;
#endif

    dev->allowOTF = true;

    dev->hwc_ctrl.max_num_ovly = NUM_HW_WINDOWS;
    dev->hwc_ctrl.num_of_video_ovly = 2;
#if (defined(USES_EXYNOS7570) || defined(USES_EXYNOS7870))
    dev->hwc_ctrl.dynamic_recomp_mode = true;
#else
    dev->hwc_ctrl.dynamic_recomp_mode = (dev->psrMode == PSR_NONE);
#endif
    dev->hwc_ctrl.skip_static_layer_mode = true;
    dev->hwc_ctrl.skip_m2m_processing_mode = true;
    dev->hwc_ctrl.dma_bw_balance_mode = true;
    dev->hwc_ctrl.skip_win_config = false;

    hwcDebug = 0;

    if (dev->hwc_ctrl.dynamic_recomp_mode == true)
        exynos5_create_update_stat_thread(dev);

    dev->mVirtualDisplayDevices = 0;

    return 0;

err_vsync:
    if (dev->psrInfoFd != NULL)
        fclose(dev->psrInfoFd);
err_vsync_stat:
    close(dev->vsync_fd);
err_hdmi_open:
    if (dev->externalDisplay->mDisplayFd > 0)
        close(dev->externalDisplay->mDisplayFd);
err_ioctl:
    close(dev->primaryDisplay->mDisplayFd);
err_open_fb:
    gralloc_close(dev->primaryDisplay->mAllocDevice);
err_get_module:
    free(dev);
    return ret;
}

int exynos5_close(hw_device_t *device)
{
    struct exynos5_hwc_composer_device_1_t *dev =
            (struct exynos5_hwc_composer_device_1_t *)device;
    pthread_kill(dev->vsync_thread, SIGTERM);
    pthread_join(dev->vsync_thread, NULL);
    if (android_atomic_acquire_load(&dev->updateThreadStatus) != 0) {
        pthread_kill(dev->update_stat_thread, SIGTERM);
        pthread_join(dev->update_stat_thread, NULL);
    }
#ifndef USES_VPP
    for (size_t i = 0; i < NUM_GSC_UNITS; i++)
        dev->primaryDisplay->mMPPs[i]->cleanupM2M();
#endif
    gralloc_close(dev->primaryDisplay->mAllocDevice);
    close(dev->vsync_fd);

#ifdef USES_VPP
    delete dev->mDisplayResourceManager;
#endif

    return 0;
}

static struct hw_module_methods_t exynos5_hwc_module_methods = {
    .open = exynos5_open,
};

hwc_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = HWC_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = HWC_HARDWARE_MODULE_ID,
        .name = "Samsung exynos5 hwcomposer module",
        .author = "Samsung LSI",
        .methods = &exynos5_hwc_module_methods,
        .dso = 0,
        .reserved = {0},
    }
};
