#include "ExynosVirtualDisplayModule.h"
#include "ExynosHWCUtils.h"

ExynosVirtualDisplayModule::ExynosVirtualDisplayModule(struct exynos5_hwc_composer_device_1_t *pdev)
    : ExynosVirtualDisplay(pdev)
{
    mGLESFormat = HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M;
    mExternalMPPDstFormat = HAL_PIXEL_FORMAT_RGBA_8888;
}

ExynosVirtualDisplayModule::~ExynosVirtualDisplayModule()
{
}

void ExynosVirtualDisplayModule::configureWriteBack(hwc_display_contents_1_t *contents, decon_win_config_data &win_data)
{
    private_handle_t *handle = private_handle_t::dynamicCast(contents->outbuf);
    decon_win_config &cfg = win_data.config[MAX_DECON_WIN];

    cfg.fd_idma[0] = handle->fd;
    cfg.fd_idma[1] = handle->fd1;
    cfg.fd_idma[2] = handle->fd2;
    cfg.idma_type = ODMA_WB;
    cfg.format = halFormatToS3CFormat(handle->format);
    cfg.dst.x = 0;
    cfg.dst.y = 0;
    cfg.dst.w = mWidth;
    cfg.dst.h = mHeight;
    cfg.dst.f_w = handle->stride;
    cfg.dst.f_h = handle->vstride;
    cfg.src.x = 0;
    cfg.src.y = 0;
    cfg.src.w = mWidth;
    cfg.src.h = mHeight;
    cfg.src.f_w = handle->stride;
    cfg.src.f_h = handle->vstride;

    cfg.state = cfg.DECON_WIN_STATE_BUFFER;
    cfg.protection = mIsSecureDRM ? 1 : 0;
}

void ExynosVirtualDisplayModule::determineBandwidthSupport(hwc_display_contents_1_t *contents)
{
    ALOGV("ExynosVirtualDisplayModule::determineBandwidthSupport");

    mAllowedOverlays = min(mAllowedOverlays, DECON_2_MAX_OVERLAY_COUNT);

    ExynosVirtualDisplay::determineBandwidthSupport(contents);
}
