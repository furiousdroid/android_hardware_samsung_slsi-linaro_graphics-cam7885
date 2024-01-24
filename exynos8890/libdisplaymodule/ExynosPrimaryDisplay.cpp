#define LOG_TAG "displaymodule"
#include "ExynosPrimaryDisplay.h"
#include "ExynosHWCModule.h"
#include "ExynosHWCUtils.h"
#include "ExynosMPPModule.h"
#include "ExynosHWCDebug.h"

ExynosPrimaryDisplay::ExynosPrimaryDisplay(int __unused numMPPs, struct exynos5_hwc_composer_device_1_t *pdev)
    :   ExynosOverlayDisplay(0, pdev)
{
    mInternalDMAs.clear();
    mCheckIDMAMPP = new ExynosMPPModule(NULL, MPP_VPP_G, 0);
}

void ExynosPrimaryDisplay::doPreProcessing(hwc_display_contents_1_t* contents)
{
    ExynosDisplay::doPreProcessing(contents);
}

ExynosPrimaryDisplay::~ExynosPrimaryDisplay()
{
    if (mCheckIDMAMPP != NULL) {
        delete mCheckIDMAMPP;
        mCheckIDMAMPP = NULL;
    }
}

bool ExynosPrimaryDisplay::isOverlaySupportedByIDMA(hwc_layer_1_t &layer, size_t index)
{
    int dst_format = INTERNAL_MPP_DST_FORMAT;
    int ret = mCheckIDMAMPP->isProcessingSupported(layer, dst_format);

    if (ret < 0) {
        mLayerInfos[index]->mCheckMPPFlag |= -ret;
        return false;
    } else {
        return true;
    }
}

void ExynosPrimaryDisplay::getIDMAMinSize(hwc_layer_1_t &layer, int *w, int *h)
{
    *w = mCheckIDMAMPP->getMinWidth(layer);
    *h = mCheckIDMAMPP->getMinHeight(layer);
}

void ExynosPrimaryDisplay::determineYuvOverlay(hwc_display_contents_1_t *contents)
{
    mHwc->hwc_ctrl.max_num_ovly = NUM_HW_WINDOWS;
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.handle) {
            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
            if (!mForceFb && !isFormatRgb(handle->format)) {
                if (handle->flags & GRALLOC_USAGE_HW_CAMERA_MASK) {
                        mHwc->hwc_ctrl.max_num_ovly = 6; // plus one additional SecureDMA window
                        break;
                    }
            }
        }
    }
    ExynosDisplay::determineYuvOverlay(contents);
}

bool ExynosPrimaryDisplay::isOverlaySupported(hwc_layer_1_t &layer, size_t index, bool useVPPOverlay, ExynosMPPModule** supportedInternalMPP, ExynosMPPModule** supportedExternalMPP)
{
    return ExynosDisplay::isOverlaySupported(layer, index, useVPPOverlay, supportedInternalMPP, supportedExternalMPP);
}
