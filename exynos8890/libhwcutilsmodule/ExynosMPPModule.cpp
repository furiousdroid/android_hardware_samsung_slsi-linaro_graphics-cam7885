#include "ExynosMPPModule.h"
#include "ExynosHWCUtils.h"

ExynosMPPModule::ExynosMPPModule()
    : ExynosMPP()
{
    if (mType == MPP_VG)
        mCanBlend = true;

    if ((mType == MPP_VGR) || (mType == MPP_MSC))
        mCanRotate = true;
    else
        mCanRotate = false;
}

ExynosMPPModule::ExynosMPPModule(ExynosDisplay *display, unsigned int mppType, unsigned int mppIndex)
    : ExynosMPP(display, mppType, mppIndex)
{
    if (mType == MPP_VG)
        mCanBlend = true;

    if ((mType == MPP_VGR) || (mType == MPP_MSC))
        mCanRotate = true;
    else
        mCanRotate = false;
}

int ExynosMPPModule::getBufferUsage(private_handle_t *srcHandle)
{
    int usage = GRALLOC_USAGE_SW_READ_NEVER |
            GRALLOC_USAGE_SW_WRITE_NEVER |
            GRALLOC_USAGE_NOZEROED |
            GRALLOC_USAGE_HW_COMPOSER;

    if (getDrmMode(srcHandle->flags) == SECURE_DRM)
        usage |= GRALLOC_USAGE_PROTECTED;

    if (srcHandle->flags & GRALLOC_USAGE_VIDEO_EXT)
        usage |= GRALLOC_USAGE_VIDEO_EXT;

    /* HACK: for distinguishing FIMD_VIDEO_region */
    if (!((usage & GRALLOC_USAGE_PROTECTED) &&
         !(usage & GRALLOC_USAGE_VIDEO_EXT))) {
        usage |= (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_HW_RENDER);
    }

    return usage;
}

bool ExynosMPPModule::checkRotationCase(hwc_layer_1_t &layer, uint8_t rotType)
{
    switch(rotType) {
    case eMPPRot1:
        if ((layer.transform == 0) || (layer.transform == HAL_TRANSFORM_FLIP_H) ||
            (layer.transform == HAL_TRANSFORM_FLIP_V) || (layer.transform == HAL_TRANSFORM_ROT_180))
            return true;
        else
            return false;
    case eMPPRot2:
        if ((layer.transform ==  HAL_TRANSFORM_ROT_90) || (layer.transform == (HAL_TRANSFORM_ROT_90 | HAL_TRANSFORM_FLIP_H)) ||
            (layer.transform == (HAL_TRANSFORM_ROT_90 | HAL_TRANSFORM_FLIP_V)) || (layer.transform == HAL_TRANSFORM_ROT_270))
            return true;
        else
            return false;
    case eMPPRot3:
        if (layer.transform == 0)
            return true;
        else
            return false;
    case eMPPRot4:
        if (layer.transform != 0)
            return true;
        else
            return false;
    case eMPPRot5:
        if ((layer.transform & HAL_TRANSFORM_ROT_90) || (layer.transform & HAL_TRANSFORM_ROT_180))
            return true;
        else
            return false;
    default:
        return false;
    }
}

int ExynosMPPModule::getMaxWidth(hwc_layer_1_t __unused &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    switch (mType) {
    case MPP_VPP_G:
        return 65535;
    case MPP_VG:
    case MPP_VGR:
        if (isFormatRgb(handle->format))
            return 65535;
        else
            return 65534;
    case MPP_MSC:
    default:
        return 8192;
    }
}

int ExynosMPPModule::getMaxHeight(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    switch (mType) {
    case MPP_VPP_G:
        return 8191;
    case MPP_VG:
    case MPP_VGR:
        if (isFormatRgb(handle->format))
            return 8191;
        else if (isFormatYUV420(handle->format))
            return 8190;
        else
            return 8190;
    case MPP_MSC:
    default:
        return 8192;
    }
}

int ExynosMPPModule::getMinWidth(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    switch (mType) {
    case MPP_VPP_G:
        return 16;
    case MPP_VG:
        if (isFormatRgb(handle->format))
            return 16;
        else
            return 32;
    case MPP_VGR:
        if (isFormatRgb(handle->format)) {
            if (checkRotationCase(layer, eMPPRot1))
                return 32;
            else if (checkRotationCase(layer, eMPPRot2))
                return 16;
            else
                return 32;
        } else if (isFormatYUV420(handle->format)) {
            if (checkRotationCase(layer, eMPPRot1))
                return 64;
            else if (checkRotationCase(layer, eMPPRot2))
                return 32;
            else
                return 64;
        } else {
            return 64;
        }
    case MPP_MSC:
        return 16;
    default:
        return 64;
    }
}

int ExynosMPPModule::getMinHeight(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    switch (mType) {
    case MPP_VPP_G:
        return 8;
    case MPP_VG:
        if (isFormatRgb(handle->format))
            return 8;
        else
            return 16;
    case MPP_VGR:
        if (isFormatRgb(handle->format)) {
            if (checkRotationCase(layer, eMPPRot1))
                return 16;
            else if (checkRotationCase(layer, eMPPRot2))
                return 32;
            else
                return 32;
        } else if (isFormatYUV420(handle->format)) {
            if (checkRotationCase(layer, eMPPRot1))
                return 32;
            else if (checkRotationCase(layer, eMPPRot2))
                return 64;
            else
                return 64;
        } else {
            return 64;
        }
    case MPP_MSC:
        return 16;
    default:
        return 64;
    }
}

int ExynosMPPModule::getSrcWidthAlign(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    switch (mType) {
    case MPP_VPP_G:
        return 1;
    case MPP_VG:
    case MPP_VGR:
        if (isFormatRgb(handle->format))
            return 1;
        else
            return 2;
    case MPP_MSC:
        if (isFormatRgb(handle->format))
            return 1;
        else if (isFormatYUV420(handle->format))
            return 2;
        else
            return 2;
    default:
        return 2;
    }
}

int ExynosMPPModule::getSrcHeightAlign(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    switch (mType) {
    case MPP_VPP_G:
        return 1;
    case MPP_VG:
    case MPP_VGR:
        if (isFormatRgb(handle->format))
            return 1;
        else
            return 2;
    case MPP_MSC:
        if (isFormatRgb(handle->format))
            return 1;
        else if (isFormatYUV420(handle->format))
            return 2;
        else
            return 2;
    default:
        return 2;
    }
}

int ExynosMPPModule::getMaxCropWidth(hwc_layer_1_t &layer)
{
    if (isCompressed(layer)) {
        if (mType == MPP_VGR)
            return 4064;
    }
    switch (mType) {
    case MPP_VPP_G:
    case MPP_VG:
        return 4096;
    case MPP_VGR:
        if (checkRotationCase(layer, eMPPRot3))
            return 4096;
        else if (checkRotationCase(layer, eMPPRot4)) {
            if (checkRotationCase(layer, eMPPRot5))
                return 2560;
            else
                return 2048;
        }
        else
            return 2048;
    case MPP_MSC:
        return 8192;
    default:
        return 2048;
    }
}

int ExynosMPPModule::getMaxCropHeight(hwc_layer_1_t &layer)
{
    switch (mType) {
    case MPP_VPP_G:
    case MPP_VG:
        return 4096;
    case MPP_VGR:
        if (checkRotationCase(layer, eMPPRot3))
            return 4096;
        else if (checkRotationCase(layer, eMPPRot4))
            return 2048;
        else
            return 2048;
    case MPP_MSC:
        return 8192;
    default:
        return 2048;
    }
}

int ExynosMPPModule::getMinCropWidth(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    switch (mType) {
    case MPP_VPP_G:
        return 16;
    case MPP_VG:
        if (isFormatRgb(handle->format))
            return 16;
        else
            return 32;
    case MPP_VGR:
        if (isFormatRgb(handle->format)) {
            if (checkRotationCase(layer, eMPPRot1))
                return 32;
            else if (checkRotationCase(layer, eMPPRot2))
                return 16;
            else
                return 32;
        } else if (isFormatYUV420(handle->format)) {
            if (checkRotationCase(layer, eMPPRot1))
                return 64;
            else if (checkRotationCase(layer, eMPPRot2))
                return 32;
            else
                return 64;
        } else
            return 64;
    case MPP_MSC:
        return 16;
    default:
        return 32;
    }
}

int ExynosMPPModule::getMinCropHeight(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    switch (mType) {
    case MPP_VPP_G:
        return 8;
    case MPP_VG:
        if (isFormatRgb(handle->format))
            return 8;
        else
            return 16;
    case MPP_VGR:
        if (isFormatRgb(handle->format)) {
            if (checkRotationCase(layer, eMPPRot1))
                return 16;
            else if (checkRotationCase(layer, eMPPRot2))
                return 32;
            else
                return 32;
        } else if (isFormatYUV420(handle->format)) {
            if (checkRotationCase(layer, eMPPRot1))
                return 32;
            else if (checkRotationCase(layer, eMPPRot2))
                return 64;
            else
                return 64;
        } else
            return 64;
    case MPP_MSC:
        return 16;
    default:
        return 32;
    }
}

int ExynosMPPModule::getCropWidthAlign(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

    if (isCompressed(layer)) {
        if (mType == MPP_VGR)
            return 2;
    }
    switch (mType) {
    case MPP_VPP_G:
        return 1;
    case MPP_VG:
    case MPP_VGR:
        if (isFormatRgb(handle->format))
            return 1;
        else if (isFormatYUV420(handle->format))
            return 2;
        else
            return 2;
    case MPP_MSC:
        if (isFormatRgb(handle->format))
            return 1;
        else if (isFormatYUV420(handle->format))
            return 2;
        else
            return 2;
    default:
        return 2;
    }
}

int ExynosMPPModule::getCropHeightAlign(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    switch (mType) {
    case MPP_VPP_G:
        return 1;
    case MPP_VG:
    case MPP_VGR:
    case MPP_MSC:
        if (isFormatRgb(handle->format))
            return 1;
        else if (isFormatYUV420(handle->format))
            return 2;
        else
            return 2;
    default:
        return 2;
    }
}

int ExynosMPPModule::getSrcXOffsetAlign(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    switch (mType) {
    case MPP_VPP_G:
        return 1;
    case MPP_VG:
    case MPP_VGR:
        if (isFormatRgb(handle->format))
            return 1;
        else if (isFormatYUV420(handle->format))
            return 2;
        else
            return 2;
    case MPP_MSC:
        if (isFormatRgb(handle->format))
            return 1;
        else if (isFormatYUV420(handle->format))
            return 1;
        else
            return 2;
    default:
        return 2;
    }
}

int ExynosMPPModule::getSrcYOffsetAlign(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    switch (mType) {
    case MPP_VPP_G:
        return 1;
    case MPP_VG:
    case MPP_VGR:
        if (isFormatRgb(handle->format))
            return 1;
        else if (isFormatYUV420(handle->format))
            return 2;
        else
            return 2;
    case MPP_MSC:
        if (isFormatRgb(handle->format))
            return 1;
        else if (isFormatYUV420(handle->format))
            return 1;
        else
            return 2;
    default:
        return 2;
    }
}

int ExynosMPPModule::getMaxDstWidth(int __unused format)
{
    switch (mType) {
    case MPP_VPP_G:
    case MPP_VG:
    case MPP_VGR:
        return 4096;
    case MPP_MSC:
        return 8192;
    default:
        return 4096;
    }
}

int ExynosMPPModule::getMaxDstHeight(int __unused format)
{
    switch (mType) {
    case MPP_VPP_G:
    case MPP_VG:
    case MPP_VGR:
        return 4096;
    case MPP_MSC:
        return 8192;
    default:
        return 4096;
    }
}

int ExynosMPPModule::getMinDstWidth(int format)
{
    switch (mType) {
    case MPP_VPP_G:
        return 16;
    case MPP_VG:
        if (isFormatRgb(format))
            return 16;
        else if (isFormatYUV420(format))
            return 32;
        else
            return 32;
    case MPP_VGR:
        return 16;
    case MPP_MSC:
    default:
        return 16;
    }
}

int ExynosMPPModule::getMinDstHeight(int format)
{
    switch (mType) {
    case MPP_VPP_G:
        return 8;
    case MPP_VG:
        if (isFormatRgb(format))
            return 8;
        else if (isFormatYUV420(format))
            return 16;
        else
            return 16;
    case MPP_VGR:
        return 8;
    case MPP_MSC:
        return 16;
    default:
        return 8;
    }
}

int ExynosMPPModule::getDstWidthAlign(int format)
{
    switch (mType) {
    case MPP_VPP_G:
        return 1;
    case MPP_VG:
    case MPP_VGR:
    case MPP_MSC:
        if (isFormatRgb(format))
            return 1;
        else if (isFormatYUV420(format))
            return 2;
        else
            return 2;
    default:
        return 2;
    }
}

int ExynosMPPModule::getDstHeightAlign(int format)
{
    switch (mType) {
    case MPP_VPP_G:
        return 1;
    case MPP_VG:
    case MPP_VGR:
    case MPP_MSC:
        if (isFormatRgb(format))
            return 1;
        else if (isFormatYUV420(format))
            return 2;
        else
            return 2;
    default:
        return 2;
    }
}
int ExynosMPPModule::getMaxDownscale()
{
    if ((mType == MPP_VPP_G) || (mType == MPP_VG))
        return 1;
    else
        return ExynosMPP::getMaxDownscale();
}

int ExynosMPPModule::getMaxDownscale(hwc_layer_1_t &layer)
{
    if ((mType == MPP_VPP_G) || (mType == MPP_VG))
        return 1;
    else
        return ExynosMPP::getMaxDownscale(layer);
}

int ExynosMPPModule::getMaxUpscale()
{
    if ((mType == MPP_VPP_G) || (mType == MPP_VG))
        return 1;
    else
        return ExynosMPP::getMaxUpscale();
}

bool ExynosMPPModule::isFormatSupportedByMPP(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B:
       if ((mType == MPP_VG) || (mType == MPP_VGR) || (mType == MPP_MSC))
            return true;
       break;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_PN:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_TILED:
       if (mType == MPP_MSC)
            return true;
       break;
    }
    return ExynosMPP::isFormatSupportedByMPP(format);
}
