#define LOG_TAG "DisplayResourceManagerModule"
#define LOG_NDEBUG 0

#include "ExynosDisplayResourceManagerModule.h"
#include "ExynosHWCModule.h"
#include "ExynosMPPModule.h"
#include "ExynosPrimaryDisplay.h"
#ifdef USES_VIRTUAL_DISPLAY
#include "ExynosVirtualDisplay.h"
#endif
#if defined(USES_SINGLE_DECON)
#include "ExynosSecondaryDisplayModule.h"
#endif

ExynosDisplayResourceManagerModule::ExynosDisplayResourceManagerModule(struct exynos5_hwc_composer_device_1_t *pdev)
    : ExynosDisplayResourceManager(pdev)
{
    /*
     * This will be enabled after VPP implementation is done
     */
    size_t num_mpp_units = sizeof(AVAILABLE_INTERNAL_MPP_UNITS)/sizeof(exynos_mpp_t);
    for (size_t i = 0; i < num_mpp_units; i++) {
        exynos_mpp_t exynos_mpp = AVAILABLE_INTERNAL_MPP_UNITS[i];
        ALOGI("internalMPP type(%d), index(%d)", exynos_mpp.type, exynos_mpp.index);
        ExynosMPPModule* exynosMPP = new ExynosMPPModule(NULL, exynos_mpp.type, exynos_mpp.index);
        exynosMPP->setAllocDevice(pdev->primaryDisplay->mAllocDevice);
        mInternalMPPs.add(exynosMPP);
    }

    num_mpp_units = sizeof(AVAILABLE_EXTERNAL_MPP_UNITS)/sizeof(exynos_mpp_t);
    for (size_t i = 0; i < num_mpp_units; i++) {
        exynos_mpp_t exynos_mpp = AVAILABLE_EXTERNAL_MPP_UNITS[i];
        ALOGI("externalMPP type(%d), index(%d)", exynos_mpp.type, exynos_mpp.index);
        ExynosMPPModule* exynosMPP = new ExynosMPPModule(NULL, exynos_mpp.type, exynos_mpp.index);
#if defined(HDMI_INCAPABLE)
        if (i != HDMI_EXT_MPP_IDX)
#endif
        exynosMPP->setAllocDevice(pdev->primaryDisplay->mAllocDevice);
        mExternalMPPs.add(exynosMPP);
    }
}

ExynosDisplayResourceManagerModule::~ExynosDisplayResourceManagerModule()
{
}

void ExynosDisplayResourceManagerModule::preAssignResource()
{
#if defined(USES_SINGLE_DECON)
    if (mHwc->secondaryDisplay->mBlanked == false)
        preAssignIntMpp(mHwc->secondaryDisplay, MPP_VPP_G);
#endif
#ifdef USES_VIRTUAL_DISPLAY
    if (mHwc->virtualDisplay->mIsWFDState) {
        preAssignIntMpp(mHwc->virtualDisplay, MPP_VPP_G);
        if (mHwc->virtualDisplay->mHasDrmSurface)
            preAssignIntMpp(mHwc->virtualDisplay, MPP_VG);
    }
#endif
}
