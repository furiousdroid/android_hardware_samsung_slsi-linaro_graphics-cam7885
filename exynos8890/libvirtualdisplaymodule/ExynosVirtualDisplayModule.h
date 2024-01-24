#ifndef EXYNOS_VIRTUAL_DISPLAY_MODULE_H
#define EXYNOS_VIRTUAL_DISPLAY_MODULE_H

#include "ExynosVirtualDisplay.h"

class ExynosVirtualDisplayModule : public ExynosVirtualDisplay {
    public:
        ExynosVirtualDisplayModule(struct exynos5_hwc_composer_device_1_t *pdev);
        ~ExynosVirtualDisplayModule();

        virtual void configureWriteBack(hwc_display_contents_1_t *contents, decon_win_config_data &win_data);
        virtual void determineBandwidthSupport(hwc_display_contents_1_t *contents);
};

#endif
