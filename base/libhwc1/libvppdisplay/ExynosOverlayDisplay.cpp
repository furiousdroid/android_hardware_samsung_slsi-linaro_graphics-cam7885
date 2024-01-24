//#define LOG_NDEBUG 0
#define LOG_TAG "display"
#include "ExynosOverlayDisplay.h"
#include "ExynosHWCUtils.h"
#include "ExynosMPPModule.h"
#include "ExynosHWCDebug.h"

#ifdef G2D_COMPOSITION
#include "ExynosG2DWrapper.h"
#endif

ExynosOverlayDisplay::ExynosOverlayDisplay(int __unused numMPPs, struct exynos5_hwc_composer_device_1_t *pdev)
    :   ExynosDisplay(EXYNOS_PRIMARY_DISPLAY, pdev)
{
    this->mHwc = pdev;
    mInternalDMAs.add(IDMA_G1);
}

void ExynosOverlayDisplay::doPreProcessing(hwc_display_contents_1_t* contents)
{
    mInternalDMAs.clear();
    mInternalDMAs.add(IDMA_G1);
    ExynosDisplay::doPreProcessing(contents);
}

ExynosOverlayDisplay::~ExynosOverlayDisplay()
{
}

int ExynosOverlayDisplay::set_dual(hwc_display_contents_1_t __unused *contentsPrimary, hwc_display_contents_1_t __unused *contentsSecondary)
{
    return -1;
}
